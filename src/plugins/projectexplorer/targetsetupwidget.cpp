// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "targetsetupwidget.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "kitmanager.h"
#include "kitoptionspage.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// -------------------------------------------------------------------------
// TargetSetupWidget
// -------------------------------------------------------------------------

TargetSetupWidget::TargetSetupWidget(Kit *k, const FilePath &projectPath) :
    m_kit(k)
{
    Q_ASSERT(m_kit);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto vboxLayout = new QVBoxLayout();
    setLayout(vboxLayout);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    m_detailsWidget = new DetailsWidget(this);
    m_detailsWidget->setUseCheckBox(true);
    m_detailsWidget->setChecked(false);
    m_detailsWidget->setSummaryFontBold(true);
    vboxLayout->addWidget(m_detailsWidget);

    auto panel = new FadingWidget(m_detailsWidget);
    auto panelLayout = new QHBoxLayout(panel);
    m_manageButton = new QPushButton(KitAspect::msgManage());
    panelLayout->addWidget(m_manageButton);
    m_detailsWidget->setToolWidget(panel);

    auto widget = new QWidget;
    auto layout = new QVBoxLayout;
    widget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    auto w = new QWidget;
    m_newBuildsLayout = new QGridLayout;
    m_newBuildsLayout->setContentsMargins(0, 0, 0, 0);
    if (HostOsInfo::isMacHost())
        m_newBuildsLayout->setSpacing(0);
    w->setLayout(m_newBuildsLayout);
    layout->addWidget(w);

    widget->setEnabled(false);
    m_detailsWidget->setWidget(widget);

    setProjectPath(projectPath);

    connect(m_detailsWidget, &DetailsWidget::checked,
            this, &TargetSetupWidget::targetCheckBoxToggled);
    connect(m_manageButton, &QAbstractButton::clicked, this, &TargetSetupWidget::manageKit);
}

Kit *TargetSetupWidget::kit() const
{
    return m_kit;
}

void TargetSetupWidget::clearKit()
{
    m_kit = nullptr;
}

bool TargetSetupWidget::isKitSelected() const
{
    if (!m_kit || !m_detailsWidget->isChecked())
        return false;

    return !selectedBuildInfoList().isEmpty();
}

void TargetSetupWidget::setKitSelected(bool b)
{
    // Only check target if there are build configurations possible
    b &= hasSelectedBuildConfigurations();
    const GuardLocker locker(m_ignoreChanges);
    m_detailsWidget->setChecked(b);
    m_detailsWidget->widget()->setEnabled(b);
}

void TargetSetupWidget::addBuildInfo(const BuildInfo &info, bool isImport)
{
    QTC_ASSERT(info.kitId == m_kit->id(), return);

    if (isImport && !m_haveImported) {
        // disable everything on first import
        for (BuildInfoStore &store : m_infoStore) {
            store.isEnabled = false;
            store.checkbox->setChecked(false);
        }
        m_selected = 0;

        m_haveImported = true;
    }

    const auto pos = static_cast<int>(m_infoStore.size());

    BuildInfoStore store;
    store.buildInfo = info;
    store.isEnabled = info.enabledByDefault;
    ++m_selected;

    if (info.factory) {
        store.checkbox = new QCheckBox;
        store.checkbox->setText(info.displayName);
        store.checkbox->setChecked(store.isEnabled);
        store.checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
        m_newBuildsLayout->addWidget(store.checkbox, pos * 2, 0);

        store.pathChooser = new PathChooser();
        store.pathChooser->setExpectedKind(PathChooser::Directory);
        store.pathChooser->setFilePath(info.buildDirectory);
        if (!info.showBuildDirConfigWidget)
            store.pathChooser->setVisible(false);
        store.pathChooser->setHistoryCompleter("TargetSetup.BuildDir.History");
        store.pathChooser->setReadOnly(isImport);
        m_newBuildsLayout->addWidget(store.pathChooser, pos * 2, 1);

        store.issuesLabel = new QLabel;
        store.issuesLabel->setIndent(32);
        m_newBuildsLayout->addWidget(store.issuesLabel, pos * 2 + 1, 0, 1, 2);
        store.issuesLabel->setVisible(false);

        connect(store.checkbox, &QAbstractButton::toggled, this,
                [this, checkBox = store.checkbox](bool b) { checkBoxToggled(checkBox, b); });
        connect(store.pathChooser, &PathChooser::rawPathChanged, this,
                [this, pathChooser = store.pathChooser] { pathChanged(pathChooser); });
    }

    store.hasIssues = false;
    m_infoStore.emplace_back(std::move(store));

    reportIssues(pos);

    emit selectedToggled();
}

void TargetSetupWidget::targetCheckBoxToggled(bool b)
{
    if (m_ignoreChanges.isLocked())
        return;
    m_detailsWidget->widget()->setEnabled(b);
    if (b && (contains(m_infoStore, &BuildInfoStore::hasIssues)
              || !contains(m_infoStore, &BuildInfoStore::isEnabled))) {
        m_detailsWidget->setState(DetailsWidget::Expanded);
    } else if (!b) {
        m_detailsWidget->setState(DetailsWidget::Collapsed);
    }
    emit selectedToggled();
}

void TargetSetupWidget::manageKit()
{
    if (!m_kit)
        return;

    setSelectectKitId(m_kit->id());
    Core::ICore::showOptionsDialog(Constants::KITS_SETTINGS_PAGE_ID, parentWidget());
}

void TargetSetupWidget::setProjectPath(const FilePath &projectPath)
{
    if (!m_kit)
        return;

    m_projectPath = projectPath;
    clear();

    for (const BuildInfo &info : buildInfoList(m_kit, projectPath))
        addBuildInfo(info, false);
}

void TargetSetupWidget::expandWidget()
{
    m_detailsWidget->setState(DetailsWidget::Expanded);
}

void TargetSetupWidget::update(const TasksGenerator &generator)
{
    const Tasks tasks = generator(kit());

    m_detailsWidget->setSummaryText(kit()->displayName());
    m_detailsWidget->setIcon(kit()->isValid() ? kit()->icon() : Icons::CRITICAL.icon());

    const Task errorTask = Utils::findOrDefault(tasks, Utils::equal(&Task::type, Task::Error));

    // Kits that where the taskGenarator reports an error are not selectable, because we cannot
    // guarantee that we can handle the project sensibly (e.g. qmake project without Qt).
    if (!errorTask.isNull()) {
        toggleEnabled(false);
        m_detailsWidget->setToolTip(kit()->toHtml(tasks, ""));
        m_infoStore.clear();
        return;
    }

    toggleEnabled(true);
    updateDefaultBuildDirectories();
}

const QList<BuildInfo> TargetSetupWidget::buildInfoList(const Kit *k, const FilePath &projectPath)
{
    if (auto factory = BuildConfigurationFactory::find(k, projectPath))
        return factory->allAvailableSetups(k, projectPath);

    BuildInfo info;
    info.kitId = k->id();
    return {info};
}

bool TargetSetupWidget::hasSelectedBuildConfigurations() const
{
    return !selectedBuildInfoList().isEmpty();
}

void TargetSetupWidget::toggleEnabled(bool enabled)
{
    m_detailsWidget->widget()->setEnabled(enabled && hasSelectedBuildConfigurations());
    m_detailsWidget->setCheckable(enabled);
    m_detailsWidget->setExpandable(enabled);
    if (!enabled) {
        m_detailsWidget->setState(DetailsWidget::Collapsed);
        m_detailsWidget->setChecked(false);
    }
}

const QList<BuildInfo> TargetSetupWidget::selectedBuildInfoList() const
{
    QList<BuildInfo> result;
    for (const BuildInfoStore &store : m_infoStore) {
        if (store.isEnabled)
            result.append(store.buildInfo);
    }
    return result;
}

void TargetSetupWidget::clear()
{
    m_infoStore.clear();

    m_selected = 0;
    m_haveImported = false;

    emit selectedToggled();
}

void TargetSetupWidget::updateDefaultBuildDirectories()
{
    for (const BuildInfo &buildInfo : buildInfoList(m_kit, m_projectPath)) {
        if (!buildInfo.factory)
            continue;
        bool found = false;
        for (BuildInfoStore &buildInfoStore : m_infoStore) {
            if (buildInfoStore.buildInfo.typeName == buildInfo.typeName) {
                if (!buildInfoStore.customBuildDir) {
                    const GuardLocker locker(m_ignoreChanges);
                    buildInfoStore.pathChooser->setFilePath(buildInfo.buildDirectory);
                    buildInfoStore.pathChooser->setVisible(buildInfo.showBuildDirConfigWidget);
                }
                found = true;
                break;
            }
        }
        if (!found)  // the change of the kit may have produced more build information than before
            addBuildInfo(buildInfo, false);
    }
}

void TargetSetupWidget::checkBoxToggled(QCheckBox *checkBox, bool b)
{
    auto it = std::find_if(m_infoStore.begin(), m_infoStore.end(),
              [checkBox](const BuildInfoStore &store) { return store.checkbox == checkBox; });
    QTC_ASSERT(it != m_infoStore.end(), return);
    if (it->isEnabled == b)
        return;
    m_selected += b ? 1 : -1;
    it->isEnabled = b;
    if ((m_selected == 0 && !b) || (m_selected == 1 && b)) {
        emit selectedToggled();
        m_detailsWidget->setChecked(b);
    }
}

void TargetSetupWidget::pathChanged(PathChooser *pathChooser)
{
    if (m_ignoreChanges.isLocked())
        return;

    auto it = std::find_if(m_infoStore.begin(), m_infoStore.end(),
                           [pathChooser](const BuildInfoStore &store) {
        return store.pathChooser == pathChooser;
    });
    QTC_ASSERT(it != m_infoStore.end(), return);
    it->buildInfo.buildDirectory = pathChooser->filePath();
    it->customBuildDir = true;
    reportIssues(static_cast<int>(std::distance(m_infoStore.begin(), it)));
}

void TargetSetupWidget::reportIssues(int index)
{
    const auto size = static_cast<int>(m_infoStore.size());
    QTC_ASSERT(index >= 0 && index < size, return);

    BuildInfoStore &store = m_infoStore[static_cast<size_t>(index)];
    if (store.issuesLabel) {
        QPair<Task::TaskType, QString> issues = findIssues(store.buildInfo);
        store.issuesLabel->setText(issues.second);
        store.hasIssues = issues.first != Task::Unknown;
        store.issuesLabel->setVisible(store.hasIssues);
    }
}

QPair<Task::TaskType, QString> TargetSetupWidget::findIssues(const BuildInfo &info)
{
    if (m_projectPath.isEmpty() || !info.factory)
        return {Task::Unknown, {}};

    Tasks issues;
    if (info.factory)
        issues = info.factory->reportIssues(m_kit, m_projectPath, info.buildDirectory);

    QString text;
    Task::TaskType highestType = Task::Unknown;
    for (const Task &t : std::as_const(issues)) {
        if (!text.isEmpty())
            text.append(QLatin1String("<br>"));
        // set severity:
        QString severity;
        if (t.type == Task::Error) {
            highestType = Task::Error;
            severity = Tr::tr("<b>Error:</b> ", "Severity is Task::Error");
        } else if (t.type == Task::Warning) {
            if (highestType == Task::Unknown)
                highestType = Task::Warning;
            severity = Tr::tr("<b>Warning:</b> ", "Severity is Task::Warning");
        }
        text.append(severity + t.description());
    }
    if (!text.isEmpty())
        text = QLatin1String("<nobr>") + text;
    return {highestType, text};
}

TargetSetupWidget::BuildInfoStore::~BuildInfoStore()
{
    delete checkbox;
    delete label;
    delete issuesLabel;
    delete pathChooser;
}

TargetSetupWidget::BuildInfoStore::BuildInfoStore(TargetSetupWidget::BuildInfoStore &&other)
{
    std::swap(other.buildInfo, buildInfo);
    std::swap(other.checkbox, checkbox);
    std::swap(other.label, label);
    std::swap(other.issuesLabel, issuesLabel);
    std::swap(other.pathChooser, pathChooser);
    std::swap(other.isEnabled, isEnabled);
    std::swap(other.hasIssues, hasIssues);
}

} // namespace Internal
} // namespace ProjectExplorer

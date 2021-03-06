/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "merdeployconfiguration.h"

#include "merdeploysteps.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <utils/qtcassert.h>

#include <QLabel>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace RemoteLinux;

namespace Mer {
namespace Internal {

namespace {
const char SAILFISH_AMBIENCE_CONFIG[] = "sailfish-ambience";
const int ADD_REMOVE_SPECIAL_STEPS_DELAY_MS = 1000;
} // anonymous namespace

MerDeployConfiguration::MerDeployConfiguration(Target *parent, Core::Id id,const QString& displayName)
    : RemoteLinuxDeployConfiguration(parent, id, displayName)
{
    setDisplayName(displayName);
    setDefaultDisplayName(displayName);
    init();
}

MerDeployConfiguration::MerDeployConfiguration(Target *target, MerDeployConfiguration *source)
    : RemoteLinuxDeployConfiguration(target, source)
{
    cloneSteps(source);
    init();
}

void MerDeployConfiguration::init()
{
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(target()->project());
    QTC_ASSERT(qmakeProject, return);
    connect(qmakeProject, &QmakeProject::proFilesEvaluated, this,
            &MerMb2RpmBuildConfiguration::addRemoveSpecialSteps);
}

void MerDeployConfiguration::addRemoveSpecialSteps()
{
    // Avoid temporary changes
    m_addRemoveSpecialStepsTimer.start(ADD_REMOVE_SPECIAL_STEPS_DELAY_MS, this);
}

// Override to add specific deploy steps based on particular project configuration
void MerDeployConfiguration::doAddRemoveSpecialSteps()
{
}

void MerDeployConfiguration::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_addRemoveSpecialStepsTimer.timerId()) {
        m_addRemoveSpecialStepsTimer.stop();
        doAddRemoveSpecialSteps();
    } else {
        RemoteLinuxDeployConfiguration::timerEvent(event);
    }
}

bool MerDeployConfiguration::isAmbienceProject(Project *project)
{
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(project);
    QTC_ASSERT(qmakeProject, return false);

    QmakeProFileNode *rootNode = qmakeProject->rootProjectNode();
    return rootNode->projectType() == AuxTemplate &&
        rootNode->variableValue(ConfigVar).contains(QLatin1String(SAILFISH_AMBIENCE_CONFIG));
}

// TODO add BuildStepList::removeStep(Core::Id)
void MerDeployConfiguration::removeStep(BuildStepList *stepList, Core::Id stepId)
{
    for (int i = 0; i < stepList->count(); ++i) {
        if (stepList->at(i)->id() == stepId) {
            stepList->removeStep(i);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////

MerRpmDeployConfiguration::MerRpmDeployConfiguration(Target *parent, Core::Id id)
    : MerDeployConfiguration(parent, id,displayName())
{
    init();
}

MerRpmDeployConfiguration::MerRpmDeployConfiguration(Target *target, MerRpmDeployConfiguration *source)
    : MerDeployConfiguration(target, source)
{
    init();
}

void MerRpmDeployConfiguration::init()
{
}

QString MerRpmDeployConfiguration::displayName()
{
    return tr("Deploy As RPM Package");
}

Core::Id MerRpmDeployConfiguration::configurationId()
{
    return Core::Id("QmakeProjectManager.MerRpmDeployConfiguration");
}

void MerRpmDeployConfiguration::doAddRemoveSpecialSteps()
{
    if (isAmbienceProject(target()->project())) {
        if (!stepList()->contains(MerResetAmbienceDeployStep::stepId()))
            stepList()->appendStep(new MerResetAmbienceDeployStep(stepList()));
    } else {
        removeStep(stepList(), MerResetAmbienceDeployStep::stepId());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

MerRsyncDeployConfiguration::MerRsyncDeployConfiguration(Target *parent, Core::Id id)
    : MerDeployConfiguration(parent, id,displayName())
{
}

MerRsyncDeployConfiguration::MerRsyncDeployConfiguration(Target *target, MerRsyncDeployConfiguration *source)
    : MerDeployConfiguration(target, source)
{
}

QString MerRsyncDeployConfiguration::displayName()
{
    return tr("Deploy By Copying Binaries");
}

Core::Id MerRsyncDeployConfiguration::configurationId()
{
    return Core::Id("QmakeProjectManager.MerRSyncDeployConfiguration");
}

void MerRsyncDeployConfiguration::doAddRemoveSpecialSteps()
{
    if (isAmbienceProject(target()->project())) {
        if (!stepList()->contains(MerResetAmbienceDeployStep::stepId()))
            stepList()->appendStep(new MerResetAmbienceDeployStep(stepList()));
    } else {
        removeStep(stepList(), MerResetAmbienceDeployStep::stepId());
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//TODO:HACK
MerMb2RpmBuildConfiguration::MerMb2RpmBuildConfiguration(Target *parent, Core::Id id)
    : MerDeployConfiguration(parent, id,displayName())
{
}

MerMb2RpmBuildConfiguration::MerMb2RpmBuildConfiguration(Target *target, MerMb2RpmBuildConfiguration *source)
    : MerDeployConfiguration(target, source)
{
}

QString MerMb2RpmBuildConfiguration::displayName()
{
    return tr("Deploy By Building An RPM Package");
}

Core::Id MerMb2RpmBuildConfiguration::configurationId()
{
    return Core::Id("QmakeProjectManager.MerMb2RpmBuildConfiguration");
}

void MerMb2RpmBuildConfiguration::doAddRemoveSpecialSteps()
{
    if (isAmbienceProject(target()->project())) {
        removeStep(stepList(), MerRpmValidationStep::stepId());
    } else {
        if (!stepList()->contains(MerRpmValidationStep::stepId()))
            stepList()->appendStep(new MerRpmValidationStep(stepList()));
    }
}

////////////////////////////////////////////////////////////////////////////////////

MerRpmBuildDeployConfiguration::MerRpmBuildDeployConfiguration(Target *parent, Core::Id id)
    : MerDeployConfiguration(parent, id, displayName())
{

}

MerRpmBuildDeployConfiguration::MerRpmBuildDeployConfiguration(Target *target, MerRpmBuildDeployConfiguration *source)
    : MerDeployConfiguration(target, source)
{

}

QString MerRpmBuildDeployConfiguration::displayName()
{
    return tr("Deploy As RPM Package (RPMBUILD)");
}

Core::Id MerRpmBuildDeployConfiguration::configurationId()
{
    return Core::Id("QmakeProjectManager.MerRpmLocalDeployConfiguration");
}

} // namespace Internal
} // namespace Mer

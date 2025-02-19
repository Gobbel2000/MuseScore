/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "scorerw.h"

#include <QFile>

#include "engraving/compat/scoreaccess.h"
#include "engraving/compat/mscxcompat.h"
#include "engraving/compat/writescorehook.h"
#include "engraving/rw/xml.h"
#include "engraving/libmscore/factory.h"

#include "log.h"

using namespace mu::engraving;
using namespace Ms;

QString ScoreRW::rootPath()
{
    return QString(engraving_utests_DATA_ROOT);
}

MasterScore* ScoreRW::readScore(const QString& name, bool isAbsolutePath)
{
    QString path = isAbsolutePath ? name : (rootPath() + "/" + name);
    MasterScore* score = mu::engraving::compat::ScoreAccess::createMasterScoreWithBaseStyle();
    QFileInfo fi(path);
    score->setName(fi.completeBaseName());
    QString csl  = fi.suffix().toLower();

    ScoreLoad sl;
    Score::FileError rv;
    if (csl == "mscz" || csl == "mscx") {
        rv = compat::loadMsczOrMscx(score, path, false);
    } else {
        rv = Score::FileError::FILE_UNKNOWN_TYPE;
    }

    if (rv != Score::FileError::FILE_NO_ERROR) {
        LOGE() << "can't load score, path: " << path;
        delete score;
        score = 0;
    } else {
        for (Score* s : score->scoreList()) {
            s->doLayout();
        }
    }
    return score;
}

bool ScoreRW::saveScore(Ms::Score* score, const QString& name)
{
    QFile file(name);
    if (file.exists()) {
        file.remove();
    }

    if (!file.open(QIODevice::ReadWrite)) {
        return false;
    }
    compat::WriteScoreHook hook;
    return score->writeScore(&file, false, false, hook);
}

EngravingItem* ScoreRW::writeReadElement(EngravingItem* element)
{
    //
    // write element
    //
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    XmlWriter xml(element->score(), &buffer);
    xml.writeHeader();
    element->write(xml);
    buffer.close();

    //
    // read element
    //

    XmlReader e(buffer.buffer());
    e.readNextStartElement();
    element = Factory::createItemByName(e.name(), element->score()->dummy());
    element->read(e);
    return element;
}

bool ScoreRW::saveMimeData(QByteArray mimeData, const QString& saveName)
{
    QFile f(saveName);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }

    f.write(mimeData);
    return f.error() == QFile::NoError;
}

/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RelinkingDialog.h"
#include "LoadFilesStatusDialog.h"
#include "FixDpiDialog.h"
#include "ImageMetadataLoader.h"
#include "ProcessingIndicationWidget.h"
#include "ProjectOpeningContext.h"
#include "DebugImages.h"
#include "ErrorWidget.h"
#include "Utils.h"
#include "StageSequence.h"
#include "NewOpenProjectPanel.h"
#include "MainWindow.h"
#include "Application.h"
#include "OutOfMemoryHandler.h"
#include <config.h>
#include <QtCore/QDir>

Application::Application(int& argc, char** argv)
        : QApplication(argc, argv),
          m_currentLocale("en") {
    initTranslations();
}

bool Application::notify(QObject* receiver, QEvent* e) {
    try {
        return QApplication::notify(receiver, e);
    } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();

        return false;
    }
}

void Application::installLanguage(const QString& locale) {
    if (m_currentLocale == locale) {
        return;
    }

    if (m_translationsMap.find(locale) != m_translationsMap.end()) {
        bool loaded = m_translator.load(m_translationsMap[locale]);

        this->removeTranslator(&m_translator);
        this->installTranslator(&m_translator);

        m_currentLocale = (loaded) ? locale : "en";
    } else {
        this->removeTranslator(&m_translator);

        m_currentLocale = "en";
    }
}

const QString& Application::getCurrentLocale() const {
    return m_currentLocale;
}

std::list<QString> Application::getLanguagesList() const {
    std::list<QString> list{ "en" };
    std::transform(m_translationsMap.begin(), m_translationsMap.end(),
                   std::back_inserter(list),
                   [](const std::pair<QString, QString>& val) {
        return val.first;
    });

    return list;
}

void Application::initTranslations() {
    const QStringList translation_dirs(
            QString::fromUtf8(TRANSLATION_DIRS).split(QChar(':'), QString::SkipEmptyParts)
    );

    const QStringList language_file_filter("scantailor_*.qm");
    for (const QString& path : translation_dirs) {
        QDir dir(QDir::cleanPath(applicationDirPath() + '/' + path));
        if (dir.exists()) {
            QStringList translationFileNames = QDir(dir.path()).entryList(language_file_filter);
            for (const QString& fileName : translationFileNames) {
                QString locale(fileName);
                locale.truncate(locale.lastIndexOf('.'));
                locale.remove(0, locale.indexOf('_') + 1);

                m_translationsMap[locale] = dir.absoluteFilePath(fileName);
            }
        }
    }
}

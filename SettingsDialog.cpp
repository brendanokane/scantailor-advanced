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

#include "SettingsDialog.h"
#include "OpenGLSupport.h"
#include "Application.h"
#include <QSettings>
#include <QtWidgets/QMessageBox>
#include <tiff.h>
#include <QtCore/QDir>

SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent) {
    ui.setupUi(this);

    QSettings settings;

    if (!OpenGLSupport::supported()) {
        ui.enableOpenglCb->setChecked(false);
        ui.enableOpenglCb->setEnabled(false);
        ui.openglDeviceLabel->setEnabled(false);
        ui.openglDeviceLabel->setText(tr("Your hardware / driver don't provide the necessary features"));
    } else {
        ui.enableOpenglCb->setChecked(
                settings.value("settings/enable_opengl", false).toBool()
        );
        const QString openglDevicePattern = ui.openglDeviceLabel->text();
        ui.openglDeviceLabel->setText(openglDevicePattern.arg(OpenGLSupport::deviceName()));
    }

    ui.colorSchemeBox->addItem(tr("Dark"));
    ui.colorSchemeBox->addItem(tr("Light"));
    QString val = settings.value("settings/color_scheme", "dark").toString();
    if (val == "light") {
        ui.colorSchemeBox->setCurrentIndex(1);
    } else {
        ui.colorSchemeBox->setCurrentIndex(0);
    }

    ui.tiffCompressionBWBox->addItem(tr("None"), COMPRESSION_NONE);
    ui.tiffCompressionBWBox->addItem(tr("LZW"), COMPRESSION_LZW);
    ui.tiffCompressionBWBox->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
    ui.tiffCompressionBWBox->addItem(tr("CCITT G4"), COMPRESSION_CCITTFAX4);

    ui.tiffCompressionBWBox->setCurrentIndex(
            ui.tiffCompressionBWBox->findData(
                    settings.value("settings/bw_compression", COMPRESSION_CCITTFAX4).toInt()
            )
    );

    ui.tiffCompressionColorBox->addItem(tr("None"), COMPRESSION_NONE);
    ui.tiffCompressionColorBox->addItem(tr("LZW"), COMPRESSION_LZW);
    ui.tiffCompressionColorBox->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
    ui.tiffCompressionColorBox->addItem(tr("JPEG"), COMPRESSION_JPEG);

    ui.tiffCompressionColorBox->setCurrentIndex(
            ui.tiffCompressionColorBox->findData(
                    settings.value("settings/color_compression", COMPRESSION_LZW).toInt()
            )
    );

    if (auto* app = dynamic_cast<Application*>(qApp)) {
        for (const QString& locale : app->getLanguagesList()) {
            QString languageName = QLocale::languageToString(QLocale(locale).language());
            ui.languageBox->addItem(languageName, locale);
        }

        ui.languageBox->setCurrentIndex(
                ui.languageBox->findData(app->getCurrentLocale())
        );

        ui.languageBox->setEnabled(ui.languageBox->count() > 1);
    }

    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
    ui.AutoSaveProject->setChecked(settings.value("settings/auto_save_project").toBool());
    ui.highlightDeviationCB->setChecked(settings.value("settings/highlight_deviation", true).toBool());

    connect(
            ui.colorSchemeBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onColorSchemeChanged(int))
    );
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::commitChanges() {
    QSettings settings;
    settings.setValue("settings/enable_opengl", ui.enableOpenglCb->isChecked());
    settings.setValue("settings/auto_save_project", ui.AutoSaveProject->isChecked());
    settings.setValue("settings/highlight_deviation", ui.highlightDeviationCB->isChecked());
    if (ui.colorSchemeBox->currentIndex() == 0) {
        settings.setValue("settings/color_scheme", "dark");
    } else if (ui.colorSchemeBox->currentIndex() == 1) {
        settings.setValue("settings/color_scheme", "light");
    }

    settings.setValue("settings/bw_compression", ui.tiffCompressionBWBox->currentData().toInt());
    settings.setValue("settings/color_compression", ui.tiffCompressionColorBox->currentData().toInt());
    settings.setValue("settings/language", ui.languageBox->currentData().toString());

    emit settingsChanged();
}

void SettingsDialog::onColorSchemeChanged(int idx) {
    QMessageBox::information(
            this, tr("Information"),
            tr("ScanTailor need to be restarted to apply the color scheme changes.")
    );
}

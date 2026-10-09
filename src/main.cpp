#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <memory>

#include "app/languagemanager.h"
#include "app/notecontroller.h"
#include "app/qsettingsadapter.h"
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // Must be set before QStandardPaths / QSettings are used.
    QCoreApplication::setOrganizationName(QStringLiteral("NoteManager"));
    QCoreApplication::setApplicationName(QStringLiteral("NoteManager"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Simple note manager with SQLite storage"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption dbOption(QStringLiteral("db"),
                                QStringLiteral("Path to the SQLite database file."),
                                QStringLiteral("path"));
    parser.addOption(dbOption);
    parser.process(app);

    QString dbPath = parser.value(dbOption);
    if (dbPath.isEmpty()) {
        const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        dbPath = QDir(dataDir).filePath(QStringLiteral("notes.db"));
    } else {
        QDir().mkpath(QFileInfo(dbPath).absolutePath());
    }

    auto settings = std::make_unique<QSettingsAdapter>();
    auto language = std::make_unique<LanguageManager>();
    language->apply(settings->language());

    auto controller = std::make_unique<NoteController>(dbPath, settings->searchStrategy());
    auto window = std::make_unique<MainWindow>(*controller, *settings, *language);
    window->show();
    controller->start();

    const int result = app.exec();

    window.reset();
    controller->shutdown(); // flush every pending write before exiting
    return result;
}

// Developer tool: renders the main window with demo data to a PNG file.
//   NoteManagerScreenshot <output.png> [en|ru]
// Works headless: QT_QPA_PLATFORM=offscreen.
#include <QApplication>
#include <QElapsedTimer>
#include <QTableView>
#include <QTemporaryDir>
#include <QTimer>

#include "app/languagemanager.h"
#include "app/notecontroller.h"
#include "app/qsettingsadapter.h"
#include "ui/mainwindow.h"

static Note demo(const QString &title, const QString &content, const QString &tags)
{
    Note n;
    n.title = title;
    n.content = content;
    n.tags = tags;
    return n;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("NoteManagerShot"));
    QCoreApplication::setApplicationName(QStringLiteral("NoteManagerShot"));

    const QString output = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("screenshot.png");
    const QString lang = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("en");

    QTemporaryDir dir;
    QSettingsAdapter settings(dir.filePath(QStringLiteral("settings.ini")));
    LanguageManager language;
    language.apply(lang);

    NoteController controller(dir.filePath(QStringLiteral("demo.db")), QStringLiteral("substring"));
    MainWindow window(controller, settings, language);
    window.resize(1000, 600);
    window.show();

    int savedCount = 0;
    QObject::connect(&controller, &NoteController::noteSaved, &window, [&savedCount]() { ++savedCount; });
    controller.start();

    const bool ru = lang == QLatin1String("ru");
    controller.saveNote(demo(ru ? QString::fromUtf8("План на неделю") : QStringLiteral("Weekly plan"),
                             ru ? QString::fromUtf8("- Подготовить релиз\n- Ревью pull request'ов\n- Написать README\n\n**Важно:** не забыть про тесты.")
                                : QStringLiteral("- Prepare the release\n- Review pull requests\n- Write the README\n\n**Important:** do not forget the tests."),
                             QStringLiteral("work, plan")), QString());
    controller.saveNote(demo(ru ? QString::fromUtf8("Список покупок") : QStringLiteral("Shopping list"),
                             ru ? QString::fromUtf8("Молоко, хлеб, сыр") : QStringLiteral("Milk, bread, cheese"),
                             QStringLiteral("home")), QString());
    controller.saveNote(demo(ru ? QString::fromUtf8("Идеи для проекта") : QStringLiteral("Project ideas"),
                             ru ? QString::fromUtf8("Экспорт в Markdown, поиск по тегам, синхронизация.")
                                : QStringLiteral("Markdown export, tag search, sync."),
                             QStringLiteral("ideas")), QString());

    QElapsedTimer timer;
    timer.start();
    while (savedCount < 3 && timer.elapsed() < 5000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    auto *table = window.findChild<QTableView *>();
    timer.restart();
    while (table->model()->rowCount() < 3 && timer.elapsed() < 5000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    table->selectRow(0);
    for (int i = 0; i < 10; ++i)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    const bool ok = window.grab().save(output);
    controller.shutdown();
    return ok ? 0 : 1;
}

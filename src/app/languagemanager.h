#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTranslator>

/// Installs/removes the application translator. Switching the language posts
/// QEvent::LanguageChange to all widgets, which re-translate themselves.
class LanguageManager : public QObject {
    Q_OBJECT
public:
    explicit LanguageManager(QObject *parent = nullptr);

    static QStringList availableLanguages();

    /// Returns false if the language is unknown or its catalogue cannot be loaded.
    bool apply(const QString &language);
    QString current() const { return m_current; }

private:
    QTranslator m_translator;
    QString m_current = QStringLiteral("en");
};

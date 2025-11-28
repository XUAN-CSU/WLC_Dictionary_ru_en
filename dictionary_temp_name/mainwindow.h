#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSplitter>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QHash>
#include <QMediaPlayer>
#include <QCheckBox>
#include <QProgressBar>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onLookupWord();
    void onNetworkReply(QNetworkReply *reply);
    void onTtsReply(QNetworkReply *reply);
    void onTextChanged(const QString &text);
    void onHistoryItemClicked(QListWidgetItem *item);
    void copyToClipboard();
    void copyHistoryToClipboard();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);
#else
    void onMediaStateChanged(QMediaPlayer::State state);
    void onPlayerError(QMediaPlayer::Error error);
#endif

private:
    void setupUI();
    void downloadAndPlayAudio(const QString &text, const QString &language);
    void playAudioFile(const QString &filePath);
    void playAudioForWord(const QString &word);
    QString convertToRussian(const QString &input);
    void parseOpenRussianResponse(const QByteArray &data, const QString &word);
    QString formatMarkdownFromJson(const QString &word, const QJsonArray &translations, const QJsonArray &sentences);
    void saveWordToHistory(const QString &russianWord, const QString &definition);
    void loadHistory();
    void refreshHistoryList();

    // UI Components
    QSplitter *mainSplitter;
    QLineEdit *wordInput;
    QCheckBox *autoPlayCheckbox;
    QProgressBar *lookupProgressBar;
    QProgressBar *audioProgressBar;
    QTextEdit *resultDisplay;
    QTextEdit *historyDetailDisplay;
    QListWidget *historyList;
    QPushButton *lookupButton;
    QPushButton *copyButton;
    QPushButton *copyHistoryButton;
    QLabel *statusLabel;

    // Network
    QNetworkAccessManager *networkManager;
    QNetworkAccessManager *ttsNetworkManager;

    // Media
    QMediaPlayer *mediaPlayer;
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioOutput *audioOutput;
    #endif

    // Data
    QString historyFile;
    QString currentWord;
    QString currentMarkdown;
    QString currentDefinition;
    bool isConverting;
};

#endif // MAINWINDOW_H

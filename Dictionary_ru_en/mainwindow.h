#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QClipboard>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QLabel>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QKeyEvent>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onLookupWord();
    void onNetworkReply(QNetworkReply *reply);
    void copyToClipboard();
    void copyHistoryToClipboard();
    void onHistoryItemClicked(QListWidgetItem *item);
    void onTextChanged(const QString &text);

private:
    void setupUI();
    QString convertToRussian(const QString &input);
    void parseRussianResponse(const QByteArray &data);
    QString formatMarkdown(const QJsonArray &translations);
    void saveWordToHistory(const QString &russianWord, const QString &definition);
    void loadHistory();
    void refreshHistoryList();

    QLineEdit *wordInput;
    QTextEdit *resultDisplay;
    QTextEdit *historyDetailDisplay;
    QPushButton *lookupButton;
    QPushButton *copyButton;
    QPushButton *copyHistoryButton;
    QNetworkAccessManager *networkManager;
    QLabel *statusLabel;
    QSplitter *mainSplitter;
    QListWidget *historyList;

    QString currentDefinition;
    QString currentMarkdown;
    QString historyFile;
    bool isConverting;
};

#endif // MAINWINDOW_H

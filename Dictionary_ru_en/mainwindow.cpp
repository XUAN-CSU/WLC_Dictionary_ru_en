#include "mainwindow.h"
#include <QShowEvent>
#include <QRegularExpression>
#include <QEvent>
#include <QKeyEvent>
#include <QNetworkRequest>
#include <QCheckBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDir>
#include <QProgressBar>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QTextStream>
#include <QUrl>
#include <QProcess>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMediaPlayer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , historyFile("russian_word_history.txt")
    , isConverting(false)
    , networkManager(new QNetworkAccessManager())
    , ttsNetworkManager(new QNetworkAccessManager())
{
    setupUI();

    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);
    connect(ttsNetworkManager, &QNetworkAccessManager::finished, this, &MainWindow::onTtsReply);

    // Setup media player for audio playback
    mediaPlayer = new QMediaPlayer();

    // Qt 6 style (try this first)
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.7);
    #else
    // Qt 5 style
    mediaPlayer->setVolume(70);
    #endif

    // Connect signals
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this, &MainWindow::onPlayerError);
    #else
    connect(mediaPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &MainWindow::onPlayerError);
    connect(mediaPlayer, &QMediaPlayer::stateChanged, this, &MainWindow::onMediaStateChanged);
    #endif

    // Create word_audio directory if it doesn't exist
    QDir audioDir("word_audio");
    if (!audioDir.exists()) {
        audioDir.mkpath(".");
    }

    loadHistory();
}

MainWindow::~MainWindow()
{
    // Clean up
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    delete audioOutput;
    #endif
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    setWindowTitle("Russian-English Dictionary (OpenRussian.org)");
    setMinimumSize(900, 600);

    mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);

    // Left panel - Lookup
    QWidget *leftPanel = new QWidget(mainSplitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    wordInput = new QLineEdit(leftPanel);
    wordInput->setPlaceholderText("Type using English keyboard - characters will convert to Russian automatically...");
    wordInput->setStyleSheet("QLineEdit { padding: 8px; font-size: 14px; }");

    // Audio playback checkbox
    autoPlayCheckbox = new QCheckBox("Auto-play pronunciation after lookup", leftPanel);

    // Progress bars
    lookupProgressBar = new QProgressBar(leftPanel);
    lookupProgressBar->setVisible(false);
    lookupProgressBar->setRange(0, 0);

    audioProgressBar = new QProgressBar(leftPanel);
    audioProgressBar->setVisible(false);
    audioProgressBar->setRange(0, 0);

    QLabel *lookupLabel = new QLabel("Lookup Result - Russian to English", leftPanel);
    lookupLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    resultDisplay = new QTextEdit(leftPanel);
    resultDisplay->setReadOnly(true);
    resultDisplay->setStyleSheet("QTextEdit { background-color: #f5f5f5; padding: 10px; font-size: 12px; }");

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    lookupButton = new QPushButton("Lookup", leftPanel);
    copyButton = new QPushButton("Copy as Markdown", leftPanel);

    buttonLayout->addWidget(lookupButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();

    leftLayout->addWidget(wordInput);
    leftLayout->addWidget(autoPlayCheckbox);
    leftLayout->addWidget(lookupProgressBar);
    leftLayout->addWidget(audioProgressBar);
    leftLayout->addWidget(lookupLabel);
    leftLayout->addWidget(resultDisplay);
    leftLayout->addLayout(buttonLayout);

    // Right panel - History
    QWidget *rightPanel = new QWidget(mainSplitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QLabel *historyLabel = new QLabel("Search History", rightPanel);
    historyLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; padding: 5px; background-color: #e0e0e0; }");

    historyList = new QListWidget(rightPanel);
    historyList->setStyleSheet("QListWidget { font-size: 11px; }");

    QLabel *historyDetailLabel = new QLabel("History Detail", rightPanel);
    historyDetailLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    historyDetailDisplay = new QTextEdit(rightPanel);
    historyDetailDisplay->setReadOnly(true);
    historyDetailDisplay->setStyleSheet("QTextEdit { background-color: #f8f8f8; padding: 10px; font-size: 11px; border: 1px solid #ccc; }");

    copyHistoryButton = new QPushButton("Copy as Markdown", rightPanel);

    rightLayout->addWidget(historyLabel);
    rightLayout->addWidget(historyList);
    rightLayout->addWidget(historyDetailLabel);
    rightLayout->addWidget(historyDetailDisplay);
    rightLayout->addWidget(copyHistoryButton);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({600, 300});

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);

    statusLabel = new QLabel("Ready - Type using English keyboard, characters convert to Russian automatically", centralWidget);
    statusLabel->setStyleSheet("QLabel { color: #666; font-size: 10px; padding: 5px; background-color: #f0f0f0; }");
    mainLayout->addWidget(statusLabel);

    // Connect signals and slots
    connect(wordInput, &QLineEdit::returnPressed, this, &MainWindow::onLookupWord);
    connect(wordInput, &QLineEdit::textChanged, this, &MainWindow::onTextChanged);
    connect(lookupButton, &QPushButton::clicked, this, &MainWindow::onLookupWord);
    connect(copyButton, &QPushButton::clicked, this, &MainWindow::copyToClipboard);
    connect(copyHistoryButton, &QPushButton::clicked, this, &MainWindow::copyHistoryToClipboard);
    connect(historyList, &QListWidget::itemClicked, this, &MainWindow::onHistoryItemClicked);

    wordInput->setFocus();
}
void MainWindow::onLookupWord()
{
    QString russianWord = wordInput->text().trimmed();
    if (russianWord.isEmpty()) {
        resultDisplay->setText("Please enter a Russian word to lookup.");
        return;
    }

    currentWord = russianWord;

    // Show lookup progress
    lookupProgressBar->setVisible(true);
    statusLabel->setText("Looking up Russian word: " + russianWord);
    resultDisplay->setText("Searching OpenRussian.org...");

    // Use en.openrussian.org - the correct English interface
    QString url = QString("https://en.openrussian.org/ru/%1").arg(russianWord);

    networkManager->get(QNetworkRequest(QUrl(url)));
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    lookupProgressBar->setVisible(false);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        parseOpenRussianResponse(data, currentWord);

        // Auto-play audio if checkbox is checked
        if (autoPlayCheckbox->isChecked() && !currentWord.isEmpty()) {
            downloadAndPlayAudio(currentWord, "ru");
        }
    } else {
        resultDisplay->setText("Word not found or network error: " + reply->errorString());
        statusLabel->setText("Error");
    }
    reply->deleteLater();
}

void MainWindow::downloadAndPlayAudio(const QString &text, const QString &language)
{
    if (text.isEmpty()) return;

    // Check if audio file already exists locally
    QString safeWord = text;
    safeWord.replace(QRegularExpression("[^a-zA-Z0-9а-яА-ЯёЁ]"), "_");
    QString localAudioFile = QString("word_audio/%1_%2.mp3").arg(safeWord).arg(language);
    QFile file(localAudioFile);
    if (file.exists()) {
        // Play local audio file using Qt Multimedia
        playAudioFile(localAudioFile);
        return;
    }

    statusLabel->setText("Downloading audio pronunciation...");
    audioProgressBar->setVisible(true);

    // Encode text for URL
    QString encodedText = QUrl::toPercentEncoding(text);

    // Construct Google TTS URL
    QString url;
    if (language == "ru") {
        url = QString("https://translate.google.com/translate_tts?ie=UTF-8&tl=ru&client=tw-ob&q=%1").arg(encodedText);
    } else {
        url = QString("https://translate.google.com/translate_tts?ie=UTF-8&tl=en&client=tw-ob&q=%1").arg(encodedText);
    }

    // Set headers to mimic a real browser
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    request.setRawHeader("Referer", "https://translate.google.com/");

    // Download the audio
    ttsNetworkManager->get(request);
}

void MainWindow::onTtsReply(QNetworkReply *reply)
{
    audioProgressBar->setVisible(false);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray audioData = reply->readAll();

        // Save to word_audio folder with filename based on the word
        QString safeWord = currentWord;
        safeWord.replace(QRegularExpression("[^a-zA-Z0-9а-яА-ЯёЁ]"), "_");
        QString localAudioFile = QString("word_audio/%1_ru.mp3").arg(safeWord);

        QFile file(localAudioFile);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(audioData);
            file.close();

            // Play the audio using Qt Multimedia
            playAudioFile(localAudioFile);
        } else {
            statusLabel->setText("Error saving audio file");
        }
    } else {
        statusLabel->setText("Audio download failed: " + reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::playAudioFile(const QString &filePath)
{
    statusLabel->setText("Playing pronunciation...");

    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6
    mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
    #else
    // Qt 5
    mediaPlayer->setMedia(QUrl::fromLocalFile(filePath));
    #endif

    mediaPlayer->play();
}

// Qt 5 signal handlers
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void MainWindow::onMediaStateChanged(QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        statusLabel->setText("Playing pronunciation...");
        break;
    case QMediaPlayer::StoppedState:
        statusLabel->setText("Audio finished playing");
        break;
    case QMediaPlayer::PausedState:
        statusLabel->setText("Audio paused");
        break;
    }
}

void MainWindow::onPlayerError(QMediaPlayer::Error error)
{
    statusLabel->setText("Audio playback error: " + mediaPlayer->errorString());

    // Fallback to system playback if Qt Multimedia fails
    QString filePath = mediaPlayer->currentMedia().canonicalUrl().toLocalFile();
    if (!filePath.isEmpty()) {
        #ifdef Q_OS_WIN
        QString nativePath = QDir::toNativeSeparators(filePath);
        QString command = QString("cmd /c start \"\" \"%1\"").arg(nativePath);
        QProcess::startDetached(command);
        statusLabel->setText("Using system player as fallback...");
        #endif
    }
}
#endif

// Qt 6 signal handlers
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadedMedia:
        statusLabel->setText("Audio loaded, playing...");
        break;
    case QMediaPlayer::EndOfMedia:
        statusLabel->setText("Audio finished playing");
        break;
    case QMediaPlayer::InvalidMedia:
        statusLabel->setText("Invalid audio file");
        break;
    default:
        break;
    }
}

void MainWindow::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    statusLabel->setText("Audio playback error: " + errorString);

    // Fallback to system playback if Qt Multimedia fails
    QString filePath = mediaPlayer->source().toLocalFile();
    if (!filePath.isEmpty()) {
        #ifdef Q_OS_WIN
        QString nativePath = QDir::toNativeSeparators(filePath);
        QString command = QString("cmd /c start \"\" \"%1\"").arg(nativePath);
        QProcess::startDetached(command);
        statusLabel->setText("Using system player as fallback...");
        #endif
    }
}
#endif

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate) {
        wordInput->setFocus();
        wordInput->selectAll();
        return true;
    }
    else if (event->type() == QEvent::WindowDeactivate) {
        wordInput->clear();
        return true;
    }

    return QMainWindow::event(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    wordInput->setFocus();
    wordInput->selectAll();
}

QString MainWindow::convertToRussian(const QString &input)
{
    // English keyboard to Russian Cyrillic mapping
    QHash<QString, QString> keyboardMap = {
        // First row - lowercase
        {"q", "й"}, {"w", "ц"}, {"e", "у"}, {"r", "к"}, {"t", "е"},
        {"y", "н"}, {"u", "г"}, {"i", "ш"}, {"o", "щ"}, {"p", "з"},
        {"[", "х"}, {"]", "ъ"},

        // Second row - lowercase
        {"a", "ф"}, {"s", "ы"}, {"d", "в"}, {"f", "а"}, {"g", "п"},
        {"h", "р"}, {"j", "о"}, {"k", "л"}, {"l", "д"}, {";", "ж"},
        {"'", "э"},

        // Third row - lowercase
        {"z", "я"}, {"x", "ч"}, {"c", "с"}, {"v", "м"}, {"b", "и"},
        {"n", "т"}, {"m", "ь"}, {",", "б"}, {".", "ю"}, {"/", "."},

        // First row - uppercase
        {"Q", "Й"}, {"W", "Ц"}, {"E", "У"}, {"R", "К"}, {"T", "Е"},
        {"Y", "Н"}, {"U", "Г"}, {"I", "Ш"}, {"O", "Щ"}, {"P", "З"},
        {"{", "Х"}, {"}", "Ъ"},

        // Second row - uppercase
        {"A", "Ф"}, {"S", "Ы"}, {"D", "В"}, {"F", "А"}, {"G", "П"},
        {"H", "Р"}, {"J", "О"}, {"K", "Л"}, {"L", "Д"}, {":", "Ж"},
        {"\"", "Э"},

        // Third row - uppercase
        {"Z", "Я"}, {"X", "Ч"}, {"C", "С"}, {"V", "М"}, {"B", "И"},
        {"N", "Т"}, {"M", "Ь"}, {"<", "Б"}, {">", "Ю"}, {"?", ","},

        // Special characters
        {"`", "ё"}, {"~", "Ё"}, {"@", "\""}, {"#", "№"}, {"$", ";"},
        {"^", ":"}, {"&", "?"}
    };

    QString result;

    for (int i = 0; i < input.length(); i++) {
        QChar ch = input[i];
        QString charStr = QString(ch);

        if (keyboardMap.contains(charStr)) {
            result += keyboardMap[charStr];
        } else {
            result += ch; // Keep original character if no mapping
        }
    }

    return result;
}

void MainWindow::onTextChanged(const QString &text)
{
    if (isConverting) return;

    isConverting = true;

    // Get current cursor position
    int cursorPos = wordInput->cursorPosition();

    // Convert the text to Russian
    QString russianText = convertToRussian(text);

    // If conversion happened, update the text
    if (russianText != text) {
        wordInput->setText(russianText);

        // Try to maintain cursor position
        int newCursorPos = cursorPos;
        if (russianText.length() != text.length()) {
            // If length changed, adjust cursor position
            newCursorPos = qMax(0, qMin(russianText.length(), cursorPos));
        }
        wordInput->setCursorPosition(newCursorPos);
    }

    isConverting = false;
}

void MainWindow::parseOpenRussianResponse(const QByteArray &data, const QString &word)
{
    QString html = QString::fromUtf8(data);

    // Extract JSON data from the script tag
    QString jsonData;
    QRegularExpression jsonRegex("<script id=\"__NEXT_DATA__\" type=\"application/json\">(.*?)</script>");
    QRegularExpressionMatch jsonMatch = jsonRegex.match(html);

    if (jsonMatch.hasMatch()) {
        jsonData = jsonMatch.captured(1);

        QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8());
        if (!doc.isNull()) {
            QJsonObject root = doc.object();
            QJsonObject props = root["props"].toObject();
            QJsonObject pageProps = props["pageProps"].toObject();
            QJsonObject info = pageProps["info"].toObject();

            // Extract translations from JSON
            QJsonArray words = info["words"].toArray();
            if (!words.isEmpty()) {
                QJsonObject wordData = words[0].toObject();
                QJsonArray translations = wordData["translations"].toArray();

                // Format for display
                QString result;
                result += QString("<h2 style='color: red;'>%1</h2>").arg(word);
                result += "<h3 style='color: #2E86AB; background-color: #f0f0f0; padding: 5px;'>Translations</h3>";
                result += "<ul>";

                // Use a counter instead of indexOf
                int translationIndex = 1;
                for (const QJsonValue &transValue : translations) {
                    QJsonObject translation = transValue.toObject();
                    QJsonArray tls = translation["tls"].toArray();

                    if (!tls.isEmpty()) {
                        QString translationText = tls[0].toString();
                        result += QString("<li><b>%1</b> - %2").arg(translationIndex).arg(translationText);

                        // Add example if available
                        QString exampleRu = translation["exampleRu"].toString();
                        QString exampleTl = translation["exampleTl"].toString();
                        if (!exampleRu.isEmpty() && !exampleTl.isEmpty()) {
                            result += QString("<br><i>Example: %1 - %2</i>").arg(exampleRu, exampleTl);
                        }

                        result += "</li>";
                        translationIndex++;
                    }
                }
                result += "</ul>";

                // Extract examples
                QJsonArray sentences = wordData["sentences"].toArray();
                if (!sentences.isEmpty()) {
                    result += "<h3 style='color: #2E86AB; background-color: #f0f0f0; padding: 5px;'>Examples</h3>";
                    result += "<ul>";

                    for (int i = 0; i < sentences.size() && i < 10; ++i) {
                        QJsonObject sentence = sentences[i].toObject();
                        QString ru = sentence["ru"].toString();
                        QString tl = sentence["tl"].toString();

                        result += QString("<li><b>Russian:</b> %1<br><b>English:</b> %2</li>").arg(ru, tl);
                    }
                    result += "</ul>";
                }

                currentDefinition = result;

                // Generate markdown format
                currentMarkdown = formatMarkdownFromJson(word, translations, sentences);

                resultDisplay->setHtml(result);
                statusLabel->setText("Found - " + QDateTime::currentDateTime().toString("hh:mm:ss"));

                // Save to history
                saveWordToHistory(word, result);
                refreshHistoryList();

                // Auto-copy to clipboard
                copyToClipboard();
                return;
            }
        }
    }

    // Fallback: if JSON parsing fails, use HTML parsing
    resultDisplay->setText("Could not extract dictionary data from OpenRussian.org");
    statusLabel->setText("Parse error");
}

QString MainWindow::formatMarkdownFromJson(const QString &word, const QJsonArray &translations, const QJsonArray &sentences)
{
    QString markdown;
    markdown += QString("# <font color='red'>%1</font>\n\n").arg(word);

    // Translations section
    markdown += "## Translations\n\n";

    // Use a counter instead of indexOf
    int translationIndex = 1;
    for (const QJsonValue &transValue : translations) {
        QJsonObject translation = transValue.toObject();
        QJsonArray tls = translation["tls"].toArray();

        if (!tls.isEmpty()) {
            QString translationText = tls[0].toString();

            markdown += QString("**%1. %2**").arg(translationIndex).arg(translationText);

            // Add example if available
            QString exampleRu = translation["exampleRu"].toString();
            QString exampleTl = translation["exampleTl"].toString();
            if (!exampleRu.isEmpty() && !exampleTl.isEmpty()) {
                markdown += QString("\n   *Example: %1 → %2*").arg(exampleRu, exampleTl);
            }

            markdown += "\n\n";
            translationIndex++;
        }
    }

    // Examples section
    if (!sentences.isEmpty()) {
        markdown += "## Examples\n\n";

        for (int i = 0; i < sentences.size() && i < 10; ++i) {
            QJsonObject sentence = sentences[i].toObject();
            QString ru = sentence["ru"].toString();
            QString tl = sentence["tl"].toString();

            // Clean up the text
            ru.remove(QRegularExpression("<[^>]*>"));
            ru.replace("&#x27;", "'");
            ru = ru.simplified();

            tl.remove(QRegularExpression("<[^>]*>"));
            tl.replace("&#x27;", "'");
            tl = tl.simplified();

            markdown += QString("* **Russian:** %1\n").arg(ru);
            markdown += QString("  **English:** %1\n\n").arg(tl);
        }
    }

    return markdown;
}

void MainWindow::copyToClipboard()
{
    if (!currentMarkdown.isEmpty()) {
        QApplication::clipboard()->setText(currentMarkdown);
        statusLabel->setText("Markdown copied to clipboard - " + QDateTime::currentDateTime().toString("hh:mm:ss"));
    }
}

void MainWindow::copyHistoryToClipboard()
{
    QString historyText = historyDetailDisplay->toPlainText();
    if (!historyText.isEmpty()) {
        QString markdown = "# History Lookup\n\n";

        QListWidgetItem *currentItem = historyList->currentItem();
        if (currentItem) {
            QString itemText = currentItem->text();
            QStringList parts = itemText.split(" - ");
            if (parts.size() >= 2) {
                QString wordPart = parts[1];
                QString word = wordPart.split(":")[0].trimmed();
                markdown += QString("## <font color='red'>%1</font>\n\n").arg(word);
            }
        }

        QString plainText = historyText;
        plainText.replace(QRegularExpression("Translations"), "**Translations**");
        plainText.replace(QRegularExpression("Examples"), "**Examples**");
        markdown += plainText;

        QApplication::clipboard()->setText(markdown);
        statusLabel->setText("History markdown copied to clipboard - " + QDateTime::currentDateTime().toString("hh:mm:ss"));
    }
}

void MainWindow::saveWordToHistory(const QString &russianWord, const QString &definition)
{
    QFile file(historyFile);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        QString shortDefinition = definition;
        shortDefinition.remove(QRegularExpression("<[^>]*>"));
        shortDefinition.replace("&nbsp;", " ");
        shortDefinition = shortDefinition.left(100);

        stream << timestamp << "|" << russianWord << "|" << definition << "|" << shortDefinition << "\n";
        file.close();
    }
}

void MainWindow::loadHistory()
{
    refreshHistoryList();
}

void MainWindow::refreshHistoryList()
{
    historyList->clear();
    historyDetailDisplay->clear();

    QFile file(historyFile);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QStringList lines;

        while (!stream.atEnd()) {
            lines.prepend(stream.readLine());
        }

        for (const QString &line : lines) {
            QStringList parts = line.split("|");
            if (parts.size() >= 4) {
                QString timestamp = parts[0];
                QString word = parts[1];
                QString fullDefinition = parts[2];
                QString shortDefinition = parts[3];

                QString displayText = QString("%1 - %2: %3").arg(timestamp, word, shortDefinition);

                QListWidgetItem *item = new QListWidgetItem(displayText);
                item->setData(Qt::UserRole, word);
                item->setData(Qt::UserRole + 1, fullDefinition);
                historyList->addItem(item);
            }
        }
        file.close();
    }
}

void MainWindow::onHistoryItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    QString word = item->data(Qt::UserRole).toString();
    QString fullDefinition = item->data(Qt::UserRole + 1).toString();

    historyDetailDisplay->setHtml(fullDefinition);
    statusLabel->setText("History displayed - " + word);

    // Play audio if checkbox is checked
    if (autoPlayCheckbox->isChecked() && !word.isEmpty()) {
        playAudioForWord(word);
    }
}

void MainWindow::playAudioForWord(const QString &word)
{
    // Check if audio file exists locally in word_audio folder
    QString safeWord = word;
    safeWord.replace(QRegularExpression("[^a-zA-Z0-9а-яА-ЯёЁ]"), "_");
    QString localAudioFile = QString("word_audio/%1_ru.mp3").arg(safeWord);

    QFile file(localAudioFile);
    if (file.exists()) {
        // Play local audio file
        playAudioFile(localAudioFile);
        statusLabel->setText("Playing pronunciation for: " + word);
    } else {
        // Download and play audio
        currentWord = word; // Set current word for the download
        downloadAndPlayAudio(word, "ru");
    }
}

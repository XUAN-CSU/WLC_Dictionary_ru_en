#include "mainwindow.h"
#include <QShowEvent>
#include <QRegularExpression>
#include <QEvent>
#include <QKeyEvent>
#include <QNetworkRequest>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , historyFile("russian_word_history.txt")
    , isConverting(false)  // Now this will work
{
    setupUI();
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    loadHistory();
}

MainWindow::~MainWindow()
{
}

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

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    setWindowTitle("Russian-English Dictionary");
    setMinimumSize(900, 600);

    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Left panel - Lookup
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

    wordInput = new QLineEdit(this);
    wordInput->setPlaceholderText("Type using English keyboard - characters will convert to Russian automatically...");
    wordInput->setStyleSheet("QLineEdit { padding: 8px; font-size: 14px; }");

    QLabel *lookupLabel = new QLabel("Lookup Result - Russian to English", this);
    lookupLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    resultDisplay = new QTextEdit(this);
    resultDisplay->setReadOnly(true);
    resultDisplay->setStyleSheet("QTextEdit { background-color: #f5f5f5; padding: 10px; font-size: 12px; }");

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    lookupButton = new QPushButton("Lookup", this);
    copyButton = new QPushButton("Copy as Markdown", this);

    buttonLayout->addWidget(lookupButton);
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();

    leftLayout->addWidget(wordInput);
    leftLayout->addWidget(lookupLabel);
    leftLayout->addWidget(resultDisplay);
    leftLayout->addLayout(buttonLayout);

    // Right panel - History
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QLabel *historyLabel = new QLabel("Search History", this);
    historyLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; padding: 5px; background-color: #e0e0e0; }");

    historyList = new QListWidget(this);
    historyList->setStyleSheet("QListWidget { font-size: 11px; }");

    QLabel *historyDetailLabel = new QLabel("History Detail", this);
    historyDetailLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 5px; background-color: #e0e0e0; }");

    historyDetailDisplay = new QTextEdit(this);
    historyDetailDisplay->setReadOnly(true);
    historyDetailDisplay->setStyleSheet("QTextEdit { background-color: #f8f8f8; padding: 10px; font-size: 11px; border: 1px solid #ccc; }");

    copyHistoryButton = new QPushButton("Copy as Markdown", this);

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

    statusLabel = new QLabel("Ready - Type using English keyboard, characters convert to Russian automatically", this);
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

void MainWindow::onLookupWord()
{
    QString russianWord = wordInput->text().trimmed();
    if (russianWord.isEmpty()) {
        resultDisplay->setText("Please enter a Russian word to lookup.");
        return;
    }

    statusLabel->setText("Looking up Russian word: " + russianWord);
    resultDisplay->setText("Searching...");

    // Try multiple dictionary APIs

    // Option 1: MyMemory Translation API (Free, no key needed)
    QString url = QString("https://api.mymemory.translated.net/get?q=%1&langpair=ru|en")
                     .arg(russianWord);

    // Option 2: LibreTranslate (Free, may need local instance)
    // QString url = "https://libretranslate.com/translate";
    // This would need a POST request

    // Option 3: Yandex Dictionary (Need API key)
    // QString apiKey = "YOUR_ACTUAL_YANDEX_API_KEY_HERE";
    // QString url = QString("https://dictionary.yandex.net/api/v1/dicservice.json/lookup?key=%1&lang=ru-en&text=%2")
    //                  .arg(apiKey, russianWord);

    networkManager->get(QNetworkRequest(QUrl(url)));
}


void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        parseRussianResponse(data);
    } else {
        resultDisplay->setText("Word not found or network error: " + reply->errorString());
        statusLabel->setText("Error");
    }
    reply->deleteLater();
}

void MainWindow::parseRussianResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        resultDisplay->setText("Invalid response from dictionary API.");
        statusLabel->setText("API Error");
        return;
    }

    QJsonObject response = doc.object();
    QJsonArray definitions = response["def"].toArray();

    if (definitions.isEmpty()) {
        resultDisplay->setText("Russian word not found in dictionary.");
        statusLabel->setText("Not found");
        return;
    }

    // Format for display (HTML)
    QString result;
    QString russianWord = response.value("head").toObject().value("text").toString();

    result += QString("<h2 style='color: red;'>%1</h2>").arg(russianWord);
    result += "<h3 style='color: #2E86AB; background-color: #f0f0f0; padding: 5px;'>Russian → English Translations</h3>";

    for (const QJsonValue &defValue : definitions) {
        QJsonObject definition = defValue.toObject();
        QString pos = definition["pos"].toString();
        QString posText = pos.isEmpty() ? "" : QString(" <i>(%1)</i>").arg(pos);

        QJsonArray translations = definition["tr"].toArray();
        for (int i = 0; i < translations.size() && i < 10; ++i) {
            QJsonObject translation = translations[i].toObject();
            QString englishWord = translation["text"].toString();

            result += QString("<p><b>%1.</b> %2%3").arg(i + 1).arg(englishWord, posText);

            // Add synonyms if available
            if (translation.contains("syn")) {
                QJsonArray synonyms = translation["syn"].toArray();
                if (!synonyms.isEmpty()) {
                    result += "<br><i>Synonyms: ";
                    QStringList synList;
                    for (const QJsonValue &synValue : synonyms) {
                        synList.append(synValue.toObject()["text"].toString());
                    }
                    result += synList.join(", ") + "</i>";
                }
            }

            // Add examples if available
            if (translation.contains("ex")) {
                QJsonArray examples = translation["ex"].toArray();
                for (const QJsonValue &exValue : examples) {
                    QJsonObject example = exValue.toObject();
                    QString ruEx = example["text"].toString();
                    QString enEx = example.value("tr").toArray().first().toObject()["text"].toString();
                    result += QString("<br><i>Example: %1 → %2</i>").arg(ruEx, enEx);
                }
            }

            result += "</p>";
        }
    }

    // Generate markdown format
    currentMarkdown = formatMarkdown(definitions);
    currentDefinition = result;

    resultDisplay->setHtml(result);
    statusLabel->setText("Found - " + QDateTime::currentDateTime().toString("hh:mm:ss"));

    // Save to history
    saveWordToHistory(russianWord, result);
    refreshHistoryList();

    // Auto-copy to clipboard
    copyToClipboard();
}

QString MainWindow::formatMarkdown(const QJsonArray &translations)
{
    QString markdown;

    if (translations.isEmpty()) {
        return "# No translations found\n";
    }

    // Get the Russian word from the first definition
    QString russianWord = translations.first().toObject().value("text").toString();
    markdown += QString("# <font color='red'>%1</font>\n\n").arg(russianWord);
    markdown += "## Russian → English Translations\n\n";

    int counter = 1;
    for (const QJsonValue &defValue : translations) {
        QJsonObject definition = defValue.toObject();
        QString pos = definition["pos"].toString();
        QString posText = pos.isEmpty() ? "" : QString(" (%1)").arg(pos);

        QJsonArray englishTranslations = definition["tr"].toArray();
        for (int i = 0; i < englishTranslations.size() && i < 10; ++i) {
            QJsonObject translation = englishTranslations[i].toObject();
            QString englishWord = translation["text"].toString();

            markdown += QString("%1. **%2**%3").arg(counter++).arg(englishWord, posText);

            // Add synonyms
            if (translation.contains("syn")) {
                QJsonArray synonyms = translation["syn"].toArray();
                if (!synonyms.isEmpty()) {
                    QStringList synList;
                    for (const QJsonValue &synValue : synonyms) {
                        synList.append(synValue.toObject()["text"].toString());
                    }
                    markdown += QString("\n   *Synonyms: %1*").arg(synList.join(", "));
                }
            }

            markdown += "\n\n";
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
        plainText.replace(QRegularExpression("Russian → English Translations"), "**Russian → English Translations**");
        plainText.replace(QRegularExpression("^(\\d+\\.)"), "**$1**");
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
}

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QTextEdit>
#include <QThread>
#include <QTime>
#include <QVBoxLayout>
#include <QMessageBox>

class MyWindow;

class MyThread : public QThread {
    using QThread::QThread;

Q_OBJECT

    void run() override {
        QFile data("words.txt");
        if (data.open(QFile::ReadOnly)) {
            QTextStream stream(&data);
            QString line;

            QTime time = QTime::currentTime().addMSecs(200);
            QStringList buf;

            while (stream.readLineInto(&line)) {
                if (int x = line.indexOf(this_word[0], 0, Qt::CaseSensitive); x >= 0) {
                    bool flag = true;
                    for (int i = 1; i < this_word.length(); ++i) {
                        if (x = line.indexOf(this_word[i], ++x, Qt::CaseSensitive); x < 0) {
                            flag = false;
                            break;
                        }
                    }
                    if (flag) {
                        buf.append(line);
                    }
                    if (QTime::currentTime() > time) {
                        emit stringFound(this_word, QVector<QString>::fromList(buf));
                        buf.clear();
                        time = QTime::currentTime().addMSecs(200);
                    }
                }
                if (isInterruptionRequested())
                    return;
            }
            if (!buf.empty())
                    emit stringFound(this_word, QVector<QString>::fromList(buf));
        } else {
            emit fileNotFound();
        }
    }

    friend class MyWindow;

    QString this_word;

Q_SIGNALS:

    void stringFound(const QString &, const QVector<QString> &);

    void fileNotFound();
};

class MyWindow : public QMainWindow {
Q_OBJECT
public:
    MyWindow() : QMainWindow(nullptr) {
        setWindowTitle("Search Example");
        setMinimumSize(32, 32);

        auto wid = new QWidget;
        setCentralWidget(wid);

        auto vbox = new QVBoxLayout(wid);
        wid->setLayout(vbox);

        _textEdit = new QLineEdit(this);
        _textEdit->setClearButtonEnabled(true);
        _textEdit->setPlaceholderText("Type to search...");

        _listView = new QListWidget(this);
        _listView->setUniformItemSizes(true);

        vbox->addWidget(_textEdit, 0);
        vbox->addWidget(_listView, 1);

        _worker = new MyThread(this);

        connect(_textEdit, &QLineEdit::textChanged, this, &MyWindow::textChanged);

        connect(
                _worker, &MyThread::stringFound, this,
                [this](const QString &search, const QVector<QString> &strings) {
                    if (search == this->_textEdit->text())
                        this->_listView->addItems(QStringList::fromVector(strings));
                }, Qt::QueuedConnection);
        connect(
                _worker, &MyThread::fileNotFound, this,
                [this]() {
                    QMessageBox::warning(this, "Warning", "File not found, word.txt must be in the current dir.");
                }, Qt::QueuedConnection);
    }

    ~MyWindow() override {
        if (_worker->isRunning()) {
            _worker->requestInterruption();
            _worker->wait();
        }
    }

    QSize sizeHint() const override { return {640, 480}; }

protected:
    void textChanged(const QString &text) {

        if (_worker->isRunning()) {
            _worker->requestInterruption();
            _worker->wait();
        }
        _listView->clear();
        _worker->this_word = text;
        if (!text.isEmpty())
            _worker->start(QThread::Priority::NormalPriority);
    }

    MyThread *_worker;
    QListWidget *_listView;
    QLineEdit *_textEdit;
};

#include "main2.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MyWindow win;
    win.show();
    return QApplication::exec();
}

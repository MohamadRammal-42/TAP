#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QInputDialog>
#include <iostream>
#include <QMessageBox>
#include <QScrollArea>
#include <QTextCursor>
#include <QDateTime>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("RPG Client");
    resize(960, 720);

    hostEdit = new QLineEdit("127.0.0.1");
    portEdit = new QLineEdit("6667");
    nickEdit = new QLineEdit();
    nickEdit->setPlaceholderText("Nickname");
    connectButton = new QPushButton("Connect");
    statusLabel   = new QLabel("Disconnected");
    statusLabel->setStyleSheet("color: #b00; font-weight: bold;");

    auto *connLayout = new QHBoxLayout();
    connLayout->addWidget(new QLabel("Host:"));
    connLayout->addWidget(hostEdit, 2);
    connLayout->addWidget(new QLabel("Port:"));
    connLayout->addWidget(portEdit, 1);
    connLayout->addWidget(new QLabel("Nick:"));
    connLayout->addWidget(nickEdit, 2);
    connLayout->addWidget(connectButton);
    connLayout->addWidget(statusLabel);

    output = new QTextEdit();
    output->setReadOnly(true);
    output->setStyleSheet(
        "font-family: Consolas, 'Courier New', monospace;"
        "font-size: 13px;"
        "background-color: #1e1e1e;"
        "color: #d4d4d4;"
        "border: 1px solid #333;");

    commandInput = new QLineEdit();
    commandInput->setPlaceholderText("Enter a command, e.g. LOOK, WHO, MOVE north, ATTACK goblin ...");
    commandInput->setStyleSheet("font-family: Consolas, 'Courier New', monospace; font-size: 13px;");
    sendButton = new QPushButton("Send");

    auto *inputLayout = new QHBoxLayout();
    auto *prompt = new QLabel("&gt;");
    prompt->setStyleSheet("font-family: monospace; font-weight: bold;");
    inputLayout->addWidget(prompt);
    inputLayout->addWidget(commandInput, 1);
    inputLayout->addWidget(sendButton);

    auto *consoleTab    = new QWidget();
    auto *consoleLayout = new QVBoxLayout(consoleTab);
    consoleLayout->addWidget(output, 1);
    consoleLayout->addLayout(inputLayout);

    auto *actionsInner  = new QWidget();
    auto *actionsLayout = new QVBoxLayout(actionsInner);

    {
        auto *box  = new QGroupBox("Information");
        auto *grid = new QGridLayout(box);
        const QStringList cmds = {"LOOK", "WHO", "STATUS", "INVENTORY", "QUESTS"};
        int row = 0, col = 0;
        for (const QString &c : cmds) {
            auto *b = makeActionButton(c, c);
            grid->addWidget(b, row, col++);
            if (col == 3) {
                col = 0;
                ++row;
            }
        }
        actionsLayout->addWidget(box);
    }

    {
        auto *box  = new QGroupBox("Movement");
        auto *grid = new QGridLayout(box);
        const QStringList dirs = {"north", "south", "east", "west"};
        int row = 0, col = 0;
        for (const QString &d : dirs) {
            auto *b = makeActionButton("Move " + d, "MOVE " + d);
            grid->addWidget(b, row, col++);
            if (col == 3) {
                col = 0;
                ++row;
            }
        }
        actionsLayout->addWidget(box);
    }

    {
        auto *box  = new QGroupBox("Items");
        auto *grid = new QGridLayout(box);
        grid->addWidget(makeActionButton("Take", "TAKE", "Item name:"), 0, 0);
        grid->addWidget(makeActionButton("Drop", "DROP", "Item name:"), 0, 1);
        grid->addWidget(makeActionButton("Use", "USE", "Item name:"), 1, 0);
        grid->addWidget(makeActionButton("Unuse", "UNUSE", "Item name:"), 1, 1);
        actionsLayout->addWidget(box);
    }

    {
        auto *box  = new QGroupBox("NPC");
        auto *grid = new QGridLayout(box);
        grid->addWidget(makeActionButton("Attack", "ATTACK", "NPC name:"), 0, 0);
        grid->addWidget(makeActionButton("Talk", "TALK", "NPC name:"), 0, 1);
        grid->addWidget(makeActionButton("Get Quest", "QUEST", "NPC name:"), 1, 0);
        actionsLayout->addWidget(box);
    }

    {
        auto *box  = new QGroupBox("Chat");
        auto *grid = new QGridLayout(box);
        grid->addWidget(makeActionButton("Global Chat", "CHAT GLOBAL", "Message:"), 0, 0);
        grid->addWidget(makeActionButton("Room Chat", "CHAT ROOM", "Message:"), 0, 1);
        grid->addWidget(makeActionButton("Group Chat", "CHAT GROUP", "Message:"), 0, 2);
        actionsLayout->addWidget(box);
    }

    {
        auto *box  = new QGroupBox("Party");
        auto *grid = new QGridLayout(box);
        grid->addWidget(makeActionButton("Create", "GROUP CREATE"), 0, 0);
        grid->addWidget(makeActionButton("Join", "GROUP JOIN", "Group ID:"), 0, 1);
        grid->addWidget(makeActionButton("Leave", "GROUP LEAVE"), 0, 2);
        grid->addWidget(makeActionButton("Invite", "GROUP INVITE", "Player name:"), 0, 3);
        actionsLayout->addWidget(box);
    }

    {
        auto *box  = new QGroupBox("Session");
        auto *grid = new QGridLayout(box);
        grid->addWidget(makeActionButton("Quit", "QUIT"), 0, 0);
        actionsLayout->addWidget(box);
    }

    actionsLayout->addStretch();

    auto *actionsScroll = new QScrollArea();
    actionsScroll->setWidget(actionsInner);
    actionsScroll->setWidgetResizable(true);

    auto *tabs = new QTabWidget();
    tabs->addTab(consoleTab,  "Console");
    tabs->addTab(actionsScroll, "Actions");

    auto *central = new QWidget();
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->addLayout(connLayout);
    mainLayout->addWidget(tabs, 1);
    setCentralWidget(central);

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &MainWindow::onSocketConnected);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onSocketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onSocketError);

    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendCommand);
    connect(commandInput, &QLineEdit::returnPressed, this, &MainWindow::onSendCommand);

    setConnectedUi(false);
    appendOutput("Welcome! Fill in a nickname and click Connect.", "system");
}

MainWindow::~MainWindow() = default;

QPushButton *MainWindow::makeActionButton(const QString &label, const QString &command, const QString &prompt)
{
    auto *b = new QPushButton(label);
    if (prompt.isEmpty()) {
        connect(b, &QPushButton::clicked, this, [this, command]() { onQuickCommand(command); });
    } else {
        connect(b, &QPushButton::clicked, this, [this, command, prompt, label]() {
                    bool ok = false;
                    QString arg = QInputDialog::getText( this, label, prompt, QLineEdit::Normal, QString(), &ok);
                    if (ok && !arg.trimmed().isEmpty())
                        onQuickCommand(command + " " + arg.trimmed());
                });
    }
    return b;
}

void MainWindow::appendOutput(const QString &text, const QString &kind)
{
    QString color;
    if (kind == "sent")
        color = "#569cd6";  // blue
    else if (kind == "error")
        color = "#f44747";  // red
    else if (kind == "event")
        color = "#dcdcaa";  // yellow
    else if (kind == "system")
        color = "#808080";  // grey
    else
        color = "#d4d4d4";  // default

    QString local_text = text;
    if (local_text.startsWith('\0'))
        local_text.remove(0, 1);
    QString escaped = local_text.toHtmlEscaped();
    output->moveCursor(QTextCursor::End);
    output->insertHtml(QString("<span style=\"color:%1;\">%2</span><br/>").arg(color, escaped));
    output->moveCursor(QTextCursor::End);
}

void MainWindow::setConnectedUi(bool connected)
{
    connectButton->setText(connected ? "Disconnect" : "Connect");
    if (connected) {
        statusLabel->setText("Connected");
        statusLabel->setStyleSheet("color: #2a2; font-weight: bold;");
    } else {
        statusLabel->setText("Disconnected");
        statusLabel->setStyleSheet("color: #b00; font-weight: bold;");
    }
    commandInput->setEnabled(true);
    sendButton->setEnabled(true);
}

void MainWindow::onConnectClicked()
{
    if ((socket->state() == QAbstractSocket::ConnectedState || socket->state() == QAbstractSocket::ConnectingState))
    {
        if (loggedIn)
            appendOutput("Disconnecting...", "system");
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState)
            socket->waitForDisconnected(1500);
        return;
    }

    if (nickEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing nickname", "Please enter a nickname before connecting.");
        return;
    }

    buffer.clear();
    appendOutput(QString("Connecting to %1:%2 ...").arg(hostEdit->text(), portEdit->text()), "system");
    socket->connectToHost(hostEdit->text(), portEdit->text().toUShort());
}

void MainWindow::onSocketConnected()
{  
    sendCommand("CONNECT " + nickEdit->text().trimmed());
}

void MainWindow::onSocketDisconnected()
{
    if (loggedIn)
        appendOutput("Disconnected from server.", "system");
    setConnectedUi(false);
    loggedIn = false;
}

void MainWindow::onSocketError(QAbstractSocket::SocketError /*err*/)
{
    appendOutput("Socket error: " + socket->errorString(), "error");
}

void MainWindow::onReadyRead()
{
    buffer.append(socket->readAll());
    int nl;
    while ((nl = buffer.indexOf('\n')) != -1) {
        QString line = buffer.left(nl);
        buffer.remove(0, nl + 1);
        if (line.endsWith('\r'))
            line.chop(1);
        if (line.isEmpty())
            continue;
        QString kind = "info";
        if (line.startsWith("OK "))
            kind = "ok";
        else if (line.startsWith("ERR "))
            kind = "error";
        else if (line.startsWith("EVT "))
            kind = "event";
        if (line.startsWith("ERR 911 INVALID_NAME")) {
            socket->disconnectFromHost();
        }
        if (line.startsWith("ERR 201 NAME_IN_USE")) {
            socket->disconnectFromHost();
        }
        if (!loggedIn && line.startsWith("OK connected"))
        {
            loggedIn = true;
            setConnectedUi(true);
            appendOutput("TCP connection established.", "system");
        }
        appendOutput(line, kind);
    }
}

void MainWindow::sendCommand(const QString &cmd)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        appendOutput("Not connected.", "error");
        return;
    }
    QByteArray data = cmd.toUtf8();
    data.append("\r\n");
    socket->write(data);
    socket->flush();
    appendOutput("> " + cmd, "sent");
}

void MainWindow::onSendCommand()
{
    QString cmd = commandInput->text().trimmed();
    if (cmd.isEmpty())
        return;
    commandInput->clear();
    sendCommand(cmd);
}

void MainWindow::onQuickCommand(const QString &cmd)
{
    sendCommand(cmd);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (socket) {
        socket->abort();
    }

    event->accept();
}

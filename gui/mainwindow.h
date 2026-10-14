#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QCloseEvent>


class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);
    void onReadyRead();
    void onSendCommand();
    void onQuickCommand(const QString &cmd);

private:
    void sendCommand(const QString &cmd);
    void appendOutput(const QString &text, const QString &kind = "info");
    void setConnectedUi(bool connected);
    QPushButton *makeActionButton(const QString &label,
                                  const QString &command,
                                  const QString &prompt = QString());

    QLineEdit   *hostEdit   = nullptr;
    QLineEdit   *portEdit   = nullptr;
    QLineEdit   *nickEdit   = nullptr;
    QPushButton *connectButton = nullptr;
    QLabel      *statusLabel   = nullptr;

    QTextEdit   *output       = nullptr;
    QLineEdit   *commandInput = nullptr;
    QPushButton *sendButton   = nullptr;

    QTcpSocket *socket = nullptr;
    QString     buffer;
    bool        loggedIn = false;
};

#endif
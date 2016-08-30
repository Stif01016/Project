#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScopedPointer>
#include <QJsonObject>

namespace Ui {
    class MainWindow;
}

class QWebSocketServer;
class QWebSocket;
class DataManager;
class Configuration;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private Q_SLOTS:
    void onNewConnection();
    void processTextMessage(QString message);
    void socketDisconnected();

private:
    typedef QString (MainWindow::*cmdFunction)(const QJsonObject &);
    typedef std::map<QString, cmdFunction> MapFunction;

    Ui::MainWindow *ui_;
    QScopedPointer<QWebSocketServer> webSocketServer_;
    QScopedPointer<DataManager> dataManager_;
    QScopedPointer<Configuration> configuration_;

    QList<QWebSocket *> connections_;
    MapFunction funcMap_;

    QString cmdLogin(const QJsonObject &json);
    QString cmdGetPeoples(const QJsonObject &json);
    QString cmdGetItemsGroups(const QJsonObject &json);
    QString cmdGetItems(const QJsonObject &json);

    QString cmdGetTables(const QJsonObject &json);
    QString cmdGetTableBusy(const QJsonObject &json);

    QString cmdGetTime(const QJsonObject &json);

    void addLogInfo(const QString &text);
};

#endif // MAINWINDOW_H

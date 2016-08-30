#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "datamanager.h"
#include "configuration.h"
#include "api.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui_(new Ui::MainWindow)
{
    QString errText;

    ui_->setupUi(this);

    addLogInfo(trUtf8("Loading config..."));

    configuration_.reset(new Configuration());
    dataManager_.reset(new DataManager());
    if (dataManager_->connect(configuration_->dbName, "SYSDBA", "masterkey", errText) == true)
        addLogInfo(trUtf8("Connecting to database... ") + configuration_->dbName );
    else {
        addLogInfo(trUtf8("Error connecting to database... ") + configuration_->dbName);
        addLogInfo(errText);
        return;
    }

    webSocketServer_.reset(new QWebSocketServer("RPServer", QWebSocketServer::NonSecureMode, this));
    if (webSocketServer_->listen(QHostAddress::Any, configuration_->ws_port) == true) {
        addLogInfo(QString(trUtf8("Starting server (port %1)...").arg(configuration_->ws_port)));
        connect(webSocketServer_.data(), &QWebSocketServer::newConnection, this, &MainWindow::onNewConnection);
    }
    else
        addLogInfo(trUtf8("Error starting server..."));

    funcMap_.insert(std::make_pair(COMMAND::CMD_LOGIN,          &MainWindow::cmdLogin));
    funcMap_.insert(std::make_pair(COMMAND::CMD_GET_PEOPLES,    &MainWindow::cmdGetPeoples));
    funcMap_.insert(std::make_pair(COMMAND::CMD_ITEMS_GROUPS,   &MainWindow::cmdGetItemsGroups));
    funcMap_.insert(std::make_pair(COMMAND::CMD_ITEMS,          &MainWindow::cmdGetItems));
    funcMap_.insert(std::make_pair(COMMAND::CMD_GET_TABLES,     &MainWindow::cmdGetTables));
    funcMap_.insert(std::make_pair(COMMAND::CMD_GET_TABLE_BUSY, &MainWindow::cmdGetTableBusy));
    funcMap_.insert(std::make_pair(COMMAND::CMD_GET_TIME,       &MainWindow::cmdGetTime));
}

MainWindow::~MainWindow()
{
    delete ui_;
}

void MainWindow::addLogInfo(const QString &text)
{
    ui_->textEdit->append(text);
}

void MainWindow::onNewConnection()
{
    QWebSocket *pSocket = webSocketServer_->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &MainWindow::processTextMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &MainWindow::socketDisconnected);

    connections_ << pSocket;
}

void MainWindow::processTextMessage(QString message)
{
    QString result = "{\"res\":\"unkwnow_cmd\", \"err\":1}";

    addLogInfo(message);

    QWebSocket *pSocket = qobject_cast<QWebSocket *>(sender());
    if (pSocket) {
        QJsonDocument json = QJsonDocument::fromJson(message.toLocal8Bit());
        if (json.isObject() == true ) {
            QString cmd = json.object().value("cmd").toString();
            MapFunction::iterator it = funcMap_.find(cmd);
            if (it != funcMap_.end() )
                result = (this->*(it->second))(json.object());
        }

        addLogInfo(result);
        pSocket->sendTextMessage(result);
    }
}

void MainWindow::socketDisconnected()
{
    QWebSocket *pSocket = qobject_cast<QWebSocket *>(sender());

    if (pSocket) {
        connections_.removeAll(pSocket);
        pSocket->deleteLater();
    }
}

QString MainWindow::cmdLogin(const QJsonObject &json)
{
    QVariantMap pMap;
    QString people_password = json.value("people_password").toString();

    pMap = dataManager_->getPeople(people_password);
    pMap["err"] = pMap.size() > 0 ? ERROR::API_ERROR_NONE : ERROR::API_ERROR_LOGIN;
    pMap["res"] = COMMAND::CMD_LOGIN;

    return QJsonDocument::fromVariant(pMap).toJson();
}

QString MainWindow::cmdGetPeoples(const QJsonObject &json)
{
    Q_UNUSED(json);

    QVariantMap pMap;

    pMap["err"] = ERROR::API_ERROR_NONE;
    pMap["res"] = COMMAND::CMD_GET_PEOPLES;
    pMap["peoples"] = dataManager_->getPeoples();

    return QJsonDocument::fromVariant(pMap).toJson();
}

QString MainWindow::cmdGetItemsGroups(const QJsonObject &json)
{
    Q_UNUSED(json);

    QVariantMap pMap;

    pMap["err"] = ERROR::API_ERROR_NONE;
    pMap["res"] = COMMAND::CMD_GET_PEOPLES;
    pMap["groups"] = dataManager_->getItemsGroups();

    return QJsonDocument::fromVariant(pMap).toJson();
}

QString MainWindow::cmdGetItems(const QJsonObject &json)
{
    Q_UNUSED(json);

    QVariantMap pMap;

    pMap["err"] = ERROR::API_ERROR_NONE;
    pMap["res"] = COMMAND::CMD_GET_PEOPLES;
    pMap["items"] = dataManager_->getItems();

    return QJsonDocument::fromVariant(pMap).toJson();
}

QString MainWindow::cmdGetTables(const QJsonObject &json)
{
    Q_UNUSED(json);

    QVariantMap pMap;

    pMap["err"] = ERROR::API_ERROR_NONE;
    pMap["res"] = COMMAND::CMD_GET_TABLES;
    pMap["tables"] = dataManager_->getTables();

    return QJsonDocument::fromVariant(pMap).toJson();
}

QString MainWindow::cmdGetTableBusy(const QJsonObject &json)
{
    Q_UNUSED(json);

    QVariantMap pMap;

    pMap["err"] = ERROR::API_ERROR_NONE;
    pMap["res"] = COMMAND::CMD_GET_TABLE_BUSY;
    pMap["tables"] = dataManager_->getTableBusy();

    return QJsonDocument::fromVariant(pMap).toJson();
}

QString MainWindow::cmdGetTime(const QJsonObject &json)
{
    Q_UNUSED(json);

    QVariantMap pMap;

    pMap["err"] = ERROR::API_ERROR_NONE;
    pMap["res"] = COMMAND::CMD_GET_TIME;
    pMap["time"] = QDateTime::currentDateTime().toString(FORMAT::DATETIME_FORMAT);

    return QJsonDocument::fromVariant(pMap).toJson();
}



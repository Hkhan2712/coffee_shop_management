#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtNetwork>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    void startServer();

private slots:
    void incomingConnection();
    void processRequest(QTcpSocket *socket);
    void handleRequest(const QJsonObject &request, QTcpSocket *socket);

    // Product management
    void addProduct(const QJsonObject &request, QTcpSocket *socket);
    void editProduct(const QJsonObject &request, QTcpSocket *socket);
    void deleteProduct(const QJsonObject &request, QTcpSocket *socket);
    void getProducts(QTcpSocket *socket);

    // Order management
    void addOrder(const QJsonObject &request, QTcpSocket *socket);
    void processOrder(const QJsonObject &request, QTcpSocket *socket);
    void updateOrderStatus(const QJsonObject &request, QTcpSocket *socket);

    // Customer management
    void addCustomer(const QJsonObject &request, QTcpSocket *socket);
    void editCustomer(const QJsonObject &request, QTcpSocket *socket);
    void deleteCustomer(const QJsonObject &request, QTcpSocket *socket);
    void getCustomers(QTcpSocket *socket);

    // Revenue reporting
    void getRevenueReport(const QJsonObject &request, QTcpSocket *socket);

    // Employee management
    void addEmployee(const QJsonObject &request, QTcpSocket *socket);
    void editEmployee(const QJsonObject &request, QTcpSocket *socket);
    void deleteEmployee(const QJsonObject &request, QTcpSocket *socket);
    void getEmployees(QTcpSocket *socket);

private:
    QTcpServer *m_server;
    QSqlDatabase m_db;

    void initDatabase();
    void sendResponse(QTcpSocket *socket, const QJsonDocument &doc);
};

#endif // SERVER_H

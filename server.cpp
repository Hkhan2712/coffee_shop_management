#include "server.h"
#include <iostream>

Server::Server(QObject *parent) : QObject(parent), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &Server::incomingConnection);
    initDatabase();
}

void Server::startServer()
{
    if (!m_server->listen(QHostAddress::Any, 8080)) {
        qDebug() << "Server could not start!";
    } else {
        qDebug() << "Server started!";
    }
    qDebug() << "----------------------------------------------";
    std::cout << std::endl;
}

void Server::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName("localhost");
    m_db.setDatabaseName("coffeeshop");
    m_db.setUserName("coffee_shop_database");
    m_db.setPassword("admin");
    m_db.setConnectOptions("sslMode=DISABLED");

    if (!m_db.open()) {
        qDebug() << "Database connection failed:" << m_db.lastError().text();
    } else {
        qDebug() << "Database connected!";
        qDebug() << "----------------------------------------------";
        std::cout << std::endl;
    }
}

void Server::incomingConnection()
{
    qDebug() << "New connection received";
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [=]() {
            processRequest(socket);
        });
    }
}

void Server::processRequest(QTcpSocket *socket)
{
    QByteArray requestData = socket->readAll();
    qDebug() << "Request data received:" << requestData;

    QList<QByteArray> requestLines = requestData.split('\n');
    QString requestLine = requestLines.first();
    QStringList requestParts = requestLine.split(' ');

    if (requestParts.size() < 2) {
        qDebug() << "Invalid request format";
        return;
    }

    QString method = requestParts.at(0);
    QString endpoint = requestParts.at(1);

    qDebug() << "HTTP Method:" << method;
    qDebug() << "Endpoint requested:" << endpoint;

    QByteArray jsonPayload;
    if (method == "POST") {
        // Find the empty line that separates headers from the body
        int emptyLineIndex = requestData.indexOf("\r\n\r\n");
        if (emptyLineIndex != -1) {
            jsonPayload = requestData.mid(emptyLineIndex + 4);
        }
    }

    qDebug() << "JSON payload extracted:" << jsonPayload;

    QJsonDocument reqJson = QJsonDocument::fromJson(jsonPayload);
    QJsonObject reqObj = reqJson.object();
    reqObj["endpoint"] = endpoint;

    handleRequest(reqObj, socket);
}

void Server::handleRequest(const QJsonObject &request, QTcpSocket *socket)
{
    QString endpoint = request["endpoint"].toString();

    qDebug() << "Endpoint requested:" << endpoint;

    if (endpoint == "/api/products/add") {
        addProduct(request, socket);
    } else if (endpoint == "/api/products/edit") {
        editProduct(request, socket);
    } else if (endpoint == "/api/products/delete") {
        deleteProduct(request, socket);
    } else if (endpoint == "/api/products/get") {
        getProducts(socket);
    } else if (endpoint == "/api/orders/create") {
        addOrder(request, socket);
    } else if (endpoint == "/api/orders/process") {
        processOrder(request, socket);
    } else if (endpoint == "/api/orders/update") {
        updateOrderStatus(request, socket);
    } else if (endpoint == "/api/customers/add") {
        addCustomer(request, socket);
    } else if (endpoint == "/api/customers/edit") {
        editCustomer(request, socket);
    } else if (endpoint == "/api/customers/delete") {
        deleteCustomer(request, socket);
    } else if (endpoint == "/api/customers/get") {
        getCustomers(socket);
    } else if (endpoint == "/api/revenue/report") {
        getRevenueReport(request, socket);
    } else if (endpoint == "/api/employees/add") {
        addEmployee(request, socket);
    } else if (endpoint == "/api/employees/edit") {
        editEmployee(request, socket);
    } else if (endpoint == "/api/employees/delete") {
        deleteEmployee(request, socket);
    } else if (endpoint == "/api/employees/get") {
        getEmployees(socket);
    }
}

void Server::sendResponse(QTcpSocket *socket, const QJsonDocument &doc)
{
    QString response = "HTTP/1.1 200 OK\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Content-Type: application/json\r\n\r\n";
    response.append(doc.toJson(QJsonDocument::Compact));
    socket->write(response.toUtf8());
    socket->flush();
    socket->disconnectFromHost();
}

// Product management implementation

void Server::addProduct(const QJsonObject &request, QTcpSocket *socket)
{
    QString name = request["name"].toString();
    double price = request["price"].toDouble();
    QString imageUrl = request["image_url"].toString();
    QString description = request["description"].toString();

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO products (name, price, image_url, description) VALUES (?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(price);
    query.addBindValue(imageUrl);
    query.addBindValue(description);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::editProduct(const QJsonObject &request, QTcpSocket *socket)
{
    qDebug() << "Edit product request received";

    int id = request["id"].toInt();
    QString name = request["name"].toString();
    double price = request["price"].toDouble();
    QString imageUrl = request["image_url"].toString();
    QString description = request["description"].toString();

    qDebug() << "Product ID:" << id;
    qDebug() << "Product Name:" << name;
    qDebug() << "Product Price:" << price;
    qDebug() << "Product Image URL:" << imageUrl;
    qDebug() << "Product Description:" << description;

    QSqlQuery query(m_db);
    query.prepare("UPDATE products SET name = ?, price = ?, image_url = ?, description = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(price);
    query.addBindValue(imageUrl);
    query.addBindValue(description);
    query.addBindValue(id);

    if (query.exec()) {
        qDebug() << "Product updated successfully";
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        qDebug() << "Error updating product:" << query.lastError().text();
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }

    qDebug() << "Edit product request processing completed";
}

void Server::deleteProduct(const QJsonObject &request, QTcpSocket *socket)
{
    int id = request["id"].toInt();

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM products WHERE id = ?");
    query.addBindValue(id);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::getProducts(QTcpSocket *socket)
{
    QSqlQuery query("SELECT id, name, price, image_url, description FROM products", m_db);

    QJsonArray productsArray;
    while (query.next()) {
        QJsonObject productObj;
        productObj["id"] = query.value(0).toInt();
        productObj["name"] = query.value(1).toString();
        productObj["price"] = query.value(2).toDouble();
        productObj["image_url"] = query.value(3).toString();
        productObj["description"] = query.value(4).toString();
        productsArray.append(productObj);
    }

    QJsonObject response;
    response["status"] = "success";
    response["products"] = productsArray;
    sendResponse(socket, QJsonDocument(response));
}

// Order management implementation
void Server::addOrder(const QJsonObject &request, QTcpSocket *socket)
{
    int customerId = request["customer_id"].toInt();
    QJsonArray productsArray = request["products"].toArray();

    // Insert new order into orders table
    QSqlQuery orderQuery(m_db);
    orderQuery.prepare("INSERT INTO orders (customer_id, total, status, created_at ) VALUES (?, ?, ?, ?)");
    orderQuery.addBindValue(customerId);
    double total = 0.0;
    for (const QJsonValue &productValue : productsArray) {
        QJsonObject productObj = productValue.toObject();
        double productTotal = productObj["quantity"].toInt() * productObj["price"].toDouble();
        total += productTotal;
    }
    orderQuery.addBindValue(total);
    orderQuery.addBindValue("Pending");
    orderQuery.addBindValue(QDate::currentDate());

    if (!orderQuery.exec()) {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = orderQuery.lastError().text();
        sendResponse(socket, QJsonDocument(response));
        return;
    }

    // Get the order ID of the newly inserted order
    int orderId = orderQuery.lastInsertId().toInt();

    // Insert each product into order_items table
    QSqlQuery orderItemQuery(m_db);
    for (const QJsonValue &productValue : productsArray) {
        QJsonObject productObj = productValue.toObject();
        int productId = productObj["product_id"].toInt();
        int quantity = productObj["quantity"].toInt();
        double price = productObj["price"].toDouble();
        double productTotal = quantity * price;

        orderItemQuery.prepare("INSERT INTO order_items (order_id, product_id, quantity, price, total) VALUES (?, ?, ?, ?, ?)");
        orderItemQuery.addBindValue(orderId);
        orderItemQuery.addBindValue(productId);
        orderItemQuery.addBindValue(quantity);
        orderItemQuery.addBindValue(price);
        orderItemQuery.addBindValue(productTotal);

        if (!orderItemQuery.exec()) {
            QJsonObject response;
            response["status"] = "error";
            response["message"] = orderItemQuery.lastError().text();
            sendResponse(socket, QJsonDocument(response));
            return;
        }
    }

    QJsonObject response;
    response["status"] = "success";
    response["order_id"] = orderId;
    sendResponse(socket, QJsonDocument(response));
}

void Server::processOrder(const QJsonObject &request, QTcpSocket *socket)
{
    int orderId = request["order_id"].toInt();
    QString newStatus = request["status"].toString();

    // Fetch the order and its items
    QSqlQuery orderQuery(m_db);
    orderQuery.prepare("SELECT status FROM orders WHERE id = ?");
    orderQuery.addBindValue(orderId);

    if (!orderQuery.exec() || !orderQuery.next()) {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = "Order not found or query failed";
        sendResponse(socket, QJsonDocument(response));
        return;
    }

    QString currentStatus = orderQuery.value(0).toString();

    if (currentStatus == "Completed" || currentStatus == "Cancelled") {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = "Cannot process an order that is already completed or cancelled";
        sendResponse(socket, QJsonDocument(response));
        return;
    }

    // Update the status of the order
    QSqlQuery updateQuery(m_db);
    updateQuery.prepare("UPDATE orders SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(orderId);

    if (!updateQuery.exec()) {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = updateQuery.lastError().text();
        sendResponse(socket, QJsonDocument(response));
        return;
    }

    // If the new status is 'Completed', update inventory
    if (newStatus == "Completed") {
        QSqlQuery itemsQuery(m_db);
        itemsQuery.prepare("SELECT product_id, quantity FROM order_items WHERE order_id = ?");
        itemsQuery.addBindValue(orderId);

        if (!itemsQuery.exec()) {
            QJsonObject response;
            response["status"] = "error";
            response["message"] = itemsQuery.lastError().text();
            sendResponse(socket, QJsonDocument(response));
            return;
        }

        while (itemsQuery.next()) {
            int productId = itemsQuery.value(0).toInt();
            int quantity = itemsQuery.value(1).toInt();

            QSqlQuery inventoryQuery(m_db);
            inventoryQuery.prepare("UPDATE products SET stock = stock - ? WHERE id = ?");
            inventoryQuery.addBindValue(quantity);
            inventoryQuery.addBindValue(productId);

            if (!inventoryQuery.exec()) {
                QJsonObject response;
                response["status"] = "error";
                response["message"] = inventoryQuery.lastError().text();
                sendResponse(socket, QJsonDocument(response));
                return;
            }
        }
    }

    QJsonObject response;
    response["status"] = "success";
    sendResponse(socket, QJsonDocument(response));
}

void Server::updateOrderStatus(const QJsonObject &request, QTcpSocket *socket)
{
    int orderId = request["order_id"].toInt();
    QString newStatus = request["status"].toString();

    // Fetch the current status of the order
    QSqlQuery query(m_db);
    query.prepare("SELECT status FROM orders WHERE id = ?");
    query.addBindValue(orderId);

    if (!query.exec() || !query.next()) {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = "Order not found or query failed";
        sendResponse(socket, QJsonDocument(response));
        return;
    }

    QString currentStatus = query.value(0).toString();

    // Update the status of the order
    QSqlQuery updateQuery(m_db);
    updateQuery.prepare("UPDATE orders SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(orderId);

    if (!updateQuery.exec()) {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = updateQuery.lastError().text();
        sendResponse(socket, QJsonDocument(response));
        return;
    }

    QJsonObject response;
    response["status"] = "success";
    sendResponse(socket, QJsonDocument(response));
}


// Customer management implementation
void Server::addCustomer(const QJsonObject &request, QTcpSocket *socket)
{
    QString name = request["name"].toString();
    QString email = request["email"].toString();
    QString phone = request["phone"].toString();
    QString address = request["address"].toString();

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO customers (name, email, phone, address) VALUES (?, ?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(email);
    query.addBindValue(phone);
    query.addBindValue(address);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::editCustomer(const QJsonObject &request, QTcpSocket *socket)
{
    int id = request["id"].toInt();
    QString name = request["name"].toString();
    QString email = request["email"].toString();
    QString phone = request["phone"].toString();
    QString address = request["address"].toString();

    QSqlQuery query(m_db);
    query.prepare("UPDATE customers SET name = ?, email = ?, phone = ?, address = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(email);
    query.addBindValue(phone);
    query.addBindValue(address);
    query.addBindValue(id);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::deleteCustomer(const QJsonObject &request, QTcpSocket *socket)
{
    int id = request["id"].toInt();

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM customers WHERE id = ?");
    query.addBindValue(id);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::getCustomers(QTcpSocket *socket)
{
    QSqlQuery query("SELECT id, name, email, phone FROM customers", m_db);

    QJsonArray customersArray;
    while (query.next()) {
        QJsonObject customerObj;
        customerObj["id"] = query.value(0).toInt();
        customerObj["name"] = query.value(1).toString();
        customerObj["email"] = query.value(2).toString();
        customerObj["phone"] = query.value(3).toString();
        customersArray.append(customerObj);
    }

    QJsonObject response;
    response["status"] = "success";
    response["customers"] = customersArray;
    sendResponse(socket, QJsonDocument(response));
}

// Revenue report implementation
void Server::getRevenueReport(const QJsonObject &request, QTcpSocket *socket)
{
    QSqlQuery query("SELECT o.date, o.total FROM orders o WHERE o.status = 'Completed'", m_db);

    QJsonArray revenueArray;
    while (query.next()) {
        QJsonObject revenueObj;
        revenueObj["date"] = query.value(0).toString();
        revenueObj["total"] = query.value(1).toDouble();
        revenueArray.append(revenueObj);
    }

    QJsonObject response;
    response["status"] = "success";
    response["revenue_report"] = revenueArray;
    sendResponse(socket, QJsonDocument(response));
}

// Employee management implementation
void Server::addEmployee(const QJsonObject &request, QTcpSocket *socket)
{
    QString name = request["name"].toString();
    QString position = request["position"].toString();
    double salary = request["salary"].toDouble();

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO employees (name, position, salary) VALUES (?, ?, ?)");
    query.addBindValue(name);
    query.addBindValue(position);
    query.addBindValue(salary);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::editEmployee(const QJsonObject &request, QTcpSocket *socket)
{
    int id = request["id"].toInt();
    QString name = request["name"].toString();
    QString position = request["position"].toString();
    double salary = request["salary"].toDouble();

    QSqlQuery query(m_db);
    query.prepare("UPDATE employees SET name = ?, position = ?, salary = ? WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(position);
    query.addBindValue(salary);
    query.addBindValue(id);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::deleteEmployee(const QJsonObject &request, QTcpSocket *socket)
{
    int id = request["id"].toInt();

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM employees WHERE id = ?");
    query.addBindValue(id);

    if (query.exec()) {
        QJsonObject response;
        response["status"] = "success";
        sendResponse(socket, QJsonDocument(response));
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = query.lastError().text();
        sendResponse(socket, QJsonDocument(response));
    }
}

void Server::getEmployees(QTcpSocket *socket)
{
    QSqlQuery query("SELECT id, name, role FROM employees", m_db);

    QJsonArray employeesArray;
    while (query.next()) {
        QJsonObject employeeObj;
        employeeObj["id"] = query.value(0).toInt();
        employeeObj["name"] = query.value(1).toString();
        employeeObj["role"] = query.value(2).toString();
        employeesArray.append(employeeObj);
    }

    QJsonObject response;
    response["status"] = "success";
    response["employees"] = employeesArray;
    sendResponse(socket, QJsonDocument(response));
}

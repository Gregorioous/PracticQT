#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QCoreApplication>
#include <QIODevice>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileDialog>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QJsonArray>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->pushButtonRead, &QPushButton::clicked, this, &MainWindow::on_pushButtonRead_clicked);
    connect(ui->pushButtonWrite, &QPushButton::clicked, this, &MainWindow::on_pushButtonWrite_clicked);


    connect(ui->tableWidget, &QTableWidget::itemChanged, this, &MainWindow::on_tableWidget_itemChanged);
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this, &MainWindow::on_tableWidget_customContextMenuRequested);

    QStringList headers = {"Адрес", "Имя", "Значение"};
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonRead_clicked()
{
    disconnect(ui->pushButtonRead, &QPushButton::clicked, this, &MainWindow::on_pushButtonRead_clicked);

    QString filePath;
    if (!lastFilePath.isEmpty()) {
        filePath = QFileDialog::getOpenFileName(this, tr("Открыть файл"), lastFilePath, tr("JSON Files (*.json)"));
    } else {
        filePath = QFileDialog::getOpenFileName(this, tr("Открыть файл"), QDir::homePath(), tr("JSON Files (*.json)"));
    }

    connect(ui->pushButtonRead, &QPushButton::clicked, this, &MainWindow::on_pushButtonRead_clicked);

    if (!filePath.isEmpty()) {
        lastFilePath = filePath;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open JSON file for reading!";
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonDocument doc(QJsonDocument::fromJson(jsonData));
        QJsonObject obj = doc.object();

        if (!obj.contains("registers")) {
            qDebug() << "Неверный формат JSON файла!";
            return;
        }

        QJsonArray registersArray = obj["registers"].toArray();

        ui->tableWidget->clearContents();
        ui->tableWidget->setRowCount(0);

        for (int i = 0; i < registersArray.size(); ++i) {
            QJsonObject registerObj = registersArray[i].toObject();
            QString addressStr = registerObj["address"].toString();
            uint address = addressStr.toUInt(nullptr, 16);
            QString name = registerObj["name"].toString();
            QString valueStr = registerObj["value"].toString();
            uint value = valueStr.toUInt(nullptr, 16);

            addRegister(address, name, value);
        }
    } else {
        qDebug() << "Файл не выбран!";
    }
}

void MainWindow::on_pushButtonWrite_clicked()
{
    disconnect(ui->pushButtonWrite, &QPushButton::clicked, this, &MainWindow::on_pushButtonWrite_clicked);

    QString filePath;
    if (!lastFilePath.isEmpty()) {
        filePath = QFileDialog::getSaveFileName(this, tr("Сохранить файл"), lastFilePath, tr("JSON Files (*.json)"));
    } else {
        filePath = QFileDialog::getSaveFileName(this, tr("Сохранить файл"), QDir::homePath(), tr("JSON Files (*.json)"));
    }

    connect(ui->pushButtonWrite, &QPushButton::clicked, this, &MainWindow::on_pushButtonWrite_clicked);

    if (filePath.isEmpty()) {
        qDebug() << "Файл не выбран!";
        return;
    }

    lastFilePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open JSON file for writing!";
        return;
    }

    QJsonArray registersArray;
    int rowCount = ui->tableWidget->rowCount();
    for (int row = 0; row < rowCount; ++row)
    {
        QTableWidgetItem *addressItem = ui->tableWidget->item(row, 0);
        QTableWidgetItem *nameItem = ui->tableWidget->item(row, 1);
        QTableWidgetItem *valueItem = ui->tableWidget->item(row, 2);

        uint address = addressItem->text().toUInt(nullptr, 16);
        QString name = nameItem->text();
        uint value = valueItem->text().toUInt(nullptr, 16);

        QJsonObject registerObj;
        registerObj["address"] = QString("0x") + QString::number(address, 16).toUpper();
        registerObj["name"] = name;
        registerObj["value"] = QString("0x") + QString::number(value, 16).toUpper();

        registersArray.append(registerObj);
    }

    QJsonObject obj;
    obj["registers"] = registersArray;
    QJsonDocument doc(obj);
    file.write(doc.toJson());
    file.close();
}

void MainWindow::on_tableWidget_itemChanged(QTableWidgetItem *item)
{
    if (item->column() == 2)
    {
        uint address = ui->tableWidget->item(item->row(), 0)->text().toUInt(nullptr, 16);
        uint newValue = item->text().toUInt(nullptr, 16);
        writeRegister(address, newValue);
    }
}

void MainWindow::addRegister(uint address, const QString &name, uint value /* = 0 */)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    QTableWidgetItem *addressItem = new QTableWidgetItem(QString("0x") + QString::number(address, 16).toUpper());
    QTableWidgetItem *nameItem = new QTableWidgetItem(name);
    QTableWidgetItem *valueItem = new QTableWidgetItem(QString("0x") + QString::number(value, 16).toUpper());

    ui->tableWidget->setItem(row, 0, addressItem);
    ui->tableWidget->setItem(row, 1, nameItem);
    ui->tableWidget->setItem(row, 2, valueItem);
}

uint MainWindow::readRegister(uint address)
{
    QFile file("registers.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to open JSON file for reading!";
        return 0;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc(QJsonDocument::fromJson(jsonData));
    QJsonObject obj = doc.object();

    QString addrKey = QString::number(address, 16).toUpper();
    if (obj.contains(addrKey))
    {
        uint value = obj[addrKey].toVariant().toUInt();
        return value;
    }
    else
    {
        qDebug() << "Address not found in the JSON file!";
        return 0;
    }
}

void MainWindow::writeRegister(uint address, uint value)
{
    QFile file("registers.json");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        qDebug() << "Failed to open JSON file for writing!";
        return;
    }

    QByteArray jsonData = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(jsonData));
    QJsonObject obj = doc.object();

    QString addrKey = QString::number(address, 16).toUpper();
    obj[addrKey] = QJsonValue::fromVariant(value);

    file.resize(0);
    file.write(doc.toJson());
    file.close();
}

void MainWindow::on_tableWidget_customContextMenuRequested(const QPoint &pos)
{
    QTableWidgetItem *item = ui->tableWidget->itemAt(pos);
    if (item) {
        QMenu contextMenu(tr("Контекстное меню"), this);
        QAction *readAction = contextMenu.addAction("Прочитать");
        QAction *writeAction = contextMenu.addAction("Записать");

        connect(readAction, &QAction::triggered, this, [=]() {
            uint address = item->text().toUInt(nullptr, 16);
            uint value = readRegister(address);
            if (value != 0) {
                writeValueToFile(address, value);
            }
        });

        connect(writeAction, &QAction::triggered, this, [=]() {
            bool ok;
            uint value = QInputDialog::getInt(this, tr("Введите значение"), tr("Значение регистра:"), 0, 0, UINT_MAX, 0, &ok);
            if (ok) {
                uint address = item->text().toUInt(nullptr, 16);
                writeRegister(address, value);
                writeValueToFile(address, value);
            }
        });

        contextMenu.exec(ui->tableWidget->mapToGlobal(pos));
    }
}

void MainWindow::writeValueToFile(uint address, uint value)
{
    QFile file("register_values.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        qDebug() << "Failed to open file for writing!";
        return;
    }

    QJsonObject registerObj;
    registerObj["address"] = QString("0x") + QString::number(address, 16).toUpper();
    registerObj["value"] = QString("0x") + QString::number(value, 16).toUpper();

    QJsonDocument doc(registerObj);
    file.write(doc.toJson());
    file.write("\n");
    file.close();
}

void MainWindow::readRegisterFromContextMenu()
{
    QTableWidgetItem *item = ui->tableWidget->currentItem();
    if (item) {
        uint address = item->text().toUInt(nullptr, 16);
        uint value = readRegister(address);
    }
}

void MainWindow::writeRegisterFromContextMenu()
{
    QTableWidgetItem *item = ui->tableWidget->currentItem();
    if (item) {
        bool ok;
        uint value = QInputDialog::getInt(this, tr("Введите значение"), tr("Значение регистра:"), 0, 0, UINT_MAX, 0, &ok);
        if (ok) {
            uint address = item->text().toUInt(nullptr, 16);
            writeRegister(address, value);
        }
    }
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonRead_clicked();
    void on_pushButtonWrite_clicked();
    void on_tableWidget_itemChanged(QTableWidgetItem *item);
    void on_tableWidget_customContextMenuRequested(const QPoint &pos);
    void readRegisterFromContextMenu();
    void writeRegisterFromContextMenu();
     void writeValueToFile(uint address, uint value);

private:
    Ui::MainWindow *ui;
    QString lastFilePath;
    uint readRegister(uint address);
    void writeRegister(uint address, uint value);
    void addRegister(uint address, const QString &name, uint value = 0);
};

#endif

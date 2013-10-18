#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStringBuilder>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "settings.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    apacheLogCommon = apacheLogCommon % "(.*) (.*) (.*) ";              // ip, caca et re-caca (tirets)
//    apacheLogCommon = apacheLogCommon % "\\[(.*)\\] ";        // date
//    apacheLogCommon = apacheLogCommon % "\"(.*) (.*) (.*)\""; // method, url, protocol
//    apacheLogCommon = apacheLogCommon % " (.*) "; // http_code
//    apacheLogCommon = apacheLogCommon % "(.*) "; // nombre de bytes
//    apacheLogCommon = apacheLogCommon % "\"(.*)\" "; // referer
//    apacheLogCommon = apacheLogCommon % "\"(.*)\""; // user-agent
    apacheLogCommon = "(.*) (.*) (.*) \\[(.*)\\] \"(.*) (.*) (.*)\" (.*) (.*) \"(.*)\" \"(.*)\"";

    // 404
    model_404 = new QStandardItemModel(0,2,this);
    model_404->setHorizontalHeaderItem(0, new QStandardItem(QString("URL")));
    model_404->setHorizontalHeaderItem(1, new QStandardItem(QString("Hits")));

    connect(ui->tableView_err404->horizontalHeader(),SIGNAL(sectionClicked(int)), ui->tableView_err404, SLOT(sortByColumn(int)));
    ui->tableView_err404->setModel(model_404);

    // GoogleBot
    model_GoogleBot = new QStandardItemModel(0,2,this);
    model_GoogleBot->setHorizontalHeaderItem(0, new QStandardItem(QString("URL")));
    model_GoogleBot->setHorizontalHeaderItem(1, new QStandardItem(QString("Hits")));
    ui->tableView_GoogleBot->setModel(model_GoogleBot);

    ui->progressBar_read->setValue(0);
    ui->pushButton_save_err404->setEnabled(false);
    ui->pushButton_save_googlebot->setEnabled(false);

    statusBar()->showMessage("Ready", 3500);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnParseLogs_clicked()
{
    settings mySettings;
    appSettings = mySettings.Load();

    QString filename = QFileDialog::getOpenFileName();

//    QString filename = "/home/dodger/access.log";
    if (filename == "") {
        return;
    }
    ui->btnParseLogs->setEnabled(false);

    qDebug() << "!" << filename << "!";

    QFile file(filename);

    int total_lines = 0;
    int read_lines = 0;

    file.open(QIODevice::ReadOnly); //| QIODevice::Text)
    QTextStream in(&file);
    while(!in.atEnd())
    {
        in.readLine();
        total_lines++;
    }

    in.seek(0);
    ui->progressBar_read->setMaximum(total_lines);

    ui->tab_err404->setDisabled(!appSettings.value("action_err404"));
    ui->tab_googlebot->setDisabled(!appSettings.value("action_googlebot"));

    while (!in.atEnd()) {
        read_lines++;
        MainWindow::parse_line(in.readLine());

        if (read_lines % 1000 == 0) {
            QString message = "Ligne " % QString::number(read_lines) % " sur " % QString::number(total_lines);
            statusBar()->showMessage(message, 1000);
        }

        ui->progressBar_read->setValue(read_lines);
    }

    add_404_to_model();
    add_GoogleBot_to_model();

    ui->pushButton_save_err404->setEnabled(true);
    ui->pushButton_save_googlebot->setEnabled(true);

    ui->tableView_err404->sortByColumn(1);
    ui->tableView_err404->resizeColumnsToContents();

    ui->tableView_GoogleBot->sortByColumn(1);
    ui->tableView_GoogleBot->resizeColumnsToContents();

    qDebug() << "Lignes parsées : " << total_lines;
    qDebug() << "URLs 404 différentes : " << hash_404.count();
    qDebug() << "URLs GoogleBot différentes : " << hash_GoogleBot.count();
}

bool MainWindow::parse_line(QString line) {
    if (line.size() < 10) {                 // ligne trop courte : on ignore
        return false;
    }

    if (line.indexOf("OPTIONS") != -1) {    // ligne "OPTIONS (dummy)" : on ignore
        return false;
    }

    total_lines++;

    QString apacheLogParse = apacheLogCommon;

    QRegExp apacheLogExp(apacheLogParse);
    apacheLogExp.setMinimal(true);

    if (!apacheLogExp.isValid()) {
        qDebug() << apacheLogExp.errorString();
    }

    int res_code;
    QString useragent;

    if (line.contains (apacheLogExp)) {

        if (apacheLogParse == apacheLogCommon) {
            res_code = apacheLogExp.cap(8).toInt();
            useragent = apacheLogExp.cap(11);
        }

//        qDebug() << "9 : " << apacheLogCommonExp.cap(9);
//        qDebug() << "8 : " << apacheLogCommonExp.cap(8);
//        qDebug() << "10 : " << apacheLogCommonExp.cap(10);
//        qDebug() << "11 : " << apacheLogCommonExp.cap(11);

        if (appSettings.value("action_err404") && res_code == 404) {
            add404(apacheLogExp);
        }

        if (appSettings.value("action_googlebot") && useragent.indexOf("Googlebot") != -1) {
            addGoogleBot(apacheLogExp);
        }
    }

    return true;
}

void MainWindow::add404(QRegExp exp) {
    QString url = exp.cap(6);

    if (url.length() < 3) {
        return;
    }

    int counter;
    if(hash_404.contains(url)) {
        counter = hash_404.value(url) + 1;
    } else {
        counter = 1;
    }

    hash_404.insert(url, counter);
}

void MainWindow::addGoogleBot(QRegExp exp) {
    QString url = exp.cap(6);

    if (url.length() < 3) {
        return;
    }

    int counter;
    if(hash_GoogleBot.contains(url)) {
        counter = hash_GoogleBot.value(url) + 1;
    } else {
        counter = 1;
    }

    hash_GoogleBot.insert(url, counter);
}

void MainWindow::save_report_err404() {
    QString filename = QFileDialog::getSaveFileName();
    qDebug() << "Saving to : " << filename;

    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    QHashIterator<QString, int> i(hash_404);
    while (i.hasNext()) {
        i.next();

        out << i.key();
        out << ";";
        out << i.value();
        out << "\n";
    }

    file.close();
}

void MainWindow::save_report_googlebot() {
    QString filename = QFileDialog::getSaveFileName();
    qDebug() << "Saving to : " << filename;

    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    QHashIterator<QString, int> i(hash_GoogleBot);
    while (i.hasNext()) {
        i.next();

        out << i.key();
        out << ";";
        out << i.value();
        out << "\n";
    }

    file.close();
}

void MainWindow::add_GoogleBot_to_model() {

    QHashIterator<QString, int> i(hash_GoogleBot);
    while (i.hasNext()) {
        i.next();

        QList<QStandardItem *> listToAdd;
        QStandardItem *item1 = new QStandardItem(i.key());
        QStandardItem *item2 = new QStandardItem(QString::number(i.value()));

        listToAdd << item1 << item2;

        model_GoogleBot->appendRow(listToAdd);
    }
}

void MainWindow::add_404_to_model() {

    QHashIterator<QString, int> i(hash_404);
    while (i.hasNext()) {
        i.next();

        QList<QStandardItem *> listToAdd;
        QStandardItem *item1 = new QStandardItem(i.key());
        QStandardItem *item2 = new QStandardItem(QString::number(i.value()));

        listToAdd << item1 << item2;

        model_404->appendRow(listToAdd);
    }
}

void MainWindow::on_action_Pr_f_rences_triggered()
{
    myPrefs = new Preferences(this);
    myPrefs->show();
}

void MainWindow::on_pushButton_prefs_clicked()
{
    myPrefs = new Preferences(this);
    myPrefs->show();
}

void MainWindow::on_pushButton_save_err404_clicked()
{
    save_report_err404();
}

void MainWindow::on_pushButton_save_googlebot_clicked()
{
    save_report_googlebot();
}

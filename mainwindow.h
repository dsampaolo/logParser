#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_btnParseLogs_clicked();
    bool parse_line(QString);

    void addGoogleBot(QRegExp);
    void add_GoogleBot_to_model();

    void add404(QRegExp);
    void add_404_to_model();

    void save_report_err404();
    void save_report_googlebot();

    void on_pushButton_save_err404_clicked();
    void on_pushButton_save_googlebot_clicked();

    void on_checkBox_Bots_stateChanged(int arg1);
    void on_checkBox_err404_stateChanged(int arg1);

    void save_settings();

    void on_checkBox_GBot_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;

    QHash<QString, bool> appSettings;

    QStandardItemModel *model_404;
    QStandardItemModel *model_GoogleBot;

    QHash<QString, int> hash_404;
    QHash<QString, int> hash_GoogleBot;

    int total_lines;

    QRegExp apacheLogExp;
};

#endif // MAINWINDOW_H

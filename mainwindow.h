#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtDebug>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QStringList>
#include <QSpinBox>
#include <QMessageBox>
#include <QDialog>
#include <QByteArray>
#include <QString>
#include <QAbstractButton>
#include <QCheckBox>
#include <QThread>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QProgressBar>
#include <QIcon>
#include <QCoreApplication>
#include <QTimer>
#include <QPen>

#include <math.h>

#define n_bytes_uart 1
//#define DOUBLE_DATA_SIZE (sizeof(double)*n_bytes_uart)
#define DOUBLE_DATA_SIZE 8
#define UART_TX_DATA (2*DOUBLE_DATA_SIZE+2)
#define BYTE_DOUBLE_RECEIVED 3*DOUBLE_DATA_SIZE
#define UART_INIT_DATA_SIZE (2+4) // PRE+DEV_ID+time_stmp(4B)
#define ADC_DATA_SIZE (2+2) // Force_H+Force_L+curr_H+curr_L
//#define RXBUFFER_SIZE (UART_INIT_DATA_SIZE+ADC_DATA_SIZE+BYTE_DOUBLE_RECEIVED+1) // PRE+DEV_ID+time_stmp(4B)+Force_H+Force_L+curr_H+curr_L+toruqe_desired(8byte)+theta(8byte)+theta_e(8byte)+lastByte
#define RX_BUFFER_SIZE (UART_INIT_DATA_SIZE+ADC_DATA_SIZE+BYTE_DOUBLE_RECEIVED+1) // PRE+DEV_ID+time_stmp(4B)+Force_H+Force_L+curr_H+curr_L+toruqe_desired(8byte)+theta(8byte)+theta_e(8byte)+lastByte

//#define MAX_MOTOR_TORQUE 35
#define MAX_MOTOR_TORQUE 70 // stall torque 191 mNm (DCX 22L EB KL 9V)
#define GH_REDUCTION 28

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    int index;
    int baud;
    QString baud_string;

    QWidget window;

private slots:
    void on_actionExit_triggered();

    void on_pushButton_UpgradeList_clicked();

    void on_pushButton_Connect_clicked();

    void on_pushButton_Disconnect_clicked();

    void on_pushButton_Start_MM_clicked();

    void on_pushButton_Stop_clicked();

    void readData();

    void on_actionChooseDirectory_triggered();

    void on_pushButton_Start_MM2_clicked();

    void on_radioButton_pre_clicked();

    void on_radioButton_post_clicked();

    void on_radioButton_healty_clicked();

    void on_pushButton_startBM1_clicked();

    void on_pushButton_startBM2_clicked();

    void on_pushButton_StartPosCtrl_clicked();


private:
    Ui::MainWindow *ui;

    QStringList baud_rate_string_list = QStringList();

    QSerialPort *serial_device = nullptr;

    QString serial_device_name = nullptr;

    QString app_name = nullptr;

    //    double getSpeed();
    double getTorque();

    void SendDataToMCU(uint8_t Command, uint8_t n_perturbations_provided, QByteArray &uartTXBuffer);

    void Plot_Data();

    double getThetaDesired();

    double getRampDuration();

    double MAX_TORQUE = GH_REDUCTION*MAX_MOTOR_TORQUE; // mNm
    //    double MAX_TORQUE = 3900; // valore massimo riportato sul manuale inviato al ministero

    uint8_t flag_start = 115;
    uint8_t flag_start2 = 110;
    uint8_t flag_startBM1 = 98;
    uint8_t flag_startBM2 = 100;
    uint8_t flag_startPC1 = 112;
    uint8_t flag_stop = 116;

    bool Start;

    QDir directory;
    QString dir_path;

    bool dir_chosen = false;

    QFile file;
    QFile log_file;

    QMessageBox error_msg();

    uint8_t UART_PRE = 85;
    uint8_t UART_DEV_ID = 106;
    uint8_t CHECK = 0;
    uint32_t rejected = 0;

    union {
        double double_torque;
        uint8_t twos_complement_array_torque[DOUBLE_DATA_SIZE];
    } u;

    union{
        double torque_desired;
        uint8_t twos_complement_array_torque_desired[DOUBLE_DATA_SIZE];
    } tau_d;

    union{
        double theta_double;
        uint8_t theta_uint8[DOUBLE_DATA_SIZE];
    } theta;

    // New in version 6
    typedef union{
        double data_double;
        uint8_t data_uint8[DOUBLE_DATA_SIZE];
    } double2uint8;

    double2uint8 theta_d; // output variable
    double2uint8 ramp_duration; // (output variable)
    double2uint8 theta_e; // double data expressed in [deg] (input variable)

    double MAX_THETA = 25; // deg

    QTimer *timer_plot = nullptr;
    int plot_refresh_period = 100; //[ms]
    double time;
    int idx;

    QByteArray uartRXBuffer_append;
//    uint8_t* RXBuffer;
    //     uint8_t RX_BUFFER_SIZE;

    typedef union{
        uint32_t data_uint32;
        uint8_t data_uint8[4];
    } unit322uint8;

    unit322uint8 time_stmp;

    double sample_freq_controller = 500; // Hz -> frequenza impostata nel timer usato per controllare. (timer 2)
    double sample_freq_time = sample_freq_controller; // Hz -> used to get time in seconds
    double srate_uart = 125; // frequenza di streaming dei dati (timer 4)

    uint8_t force_measured[2];
    uint8_t current_measured[2];

    typedef enum{
        stop,
        MM1,
        MM2,
        PC1,
        MM3,
        BM1,
        BM2,
        BM3
    }status_flag;

    status_flag device_mode = stop;
};

#endif // MAINWINDOW_H

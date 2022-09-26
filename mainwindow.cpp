/* DATA: 29/10/2018
 * VERSIONE: 3.1
 * Firmware associato: v3
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // set window title
    QWidget::setWindowTitle ("PDMeter UI v9");

    // get the name of the application for the log file
    app_name = QCoreApplication::applicationName();

    serial_device = new QSerialPort(this);

    index = 0;
    baud = 0;

    ui->comboBox_Baud_List->addItem(QStringLiteral("2400"), QSerialPort::Baud2400);
    ui->comboBox_Baud_List->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->comboBox_Baud_List->addItem(QStringLiteral("57600"), QSerialPort::Baud57600);
    ui->comboBox_Baud_List->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->comboBox_Baud_List->addItem(QStringLiteral("230400"));
    ui->comboBox_Baud_List->addItem(QStringLiteral("460800"));
    ui->comboBox_Baud_List->addItem(QStringLiteral("921600"));
    ui->comboBox_Baud_List->addItem(QStringLiteral("1405000"));
    // Set default value
    ui->comboBox_Baud_List->setCurrentIndex(6);

    // find and show all serial devices available
    ui->comboBox_Device_List->clear();
    static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");
    QString description;
    QString manufacturer;
    QString serialNumber;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;

        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);
        ui->comboBox_Device_List->addItem(list.first(), list);
    }

    ui->label_Port_Info->setText("Device Disconnected");

    ui->groupBox_Port_Info->setEnabled(1);
    ui->groupBox_Connection_Management->setEnabled(0);
    ui->groupBox_Pairing->setEnabled(0);
    ui->pushButton_Disconnect->setEnabled(0);
    ui->groupBox_Control_Centre->setEnabled(0);
    ui->groupBox_PatientID->setEnabled(0);

    ui->doubleSpinBox_Torque->setRange(0, MAX_TORQUE);
    ui->doubleSpinBox_Torque->setValue(0);

    ui->label_Torque_Info->setText(tr("Max torque: %1 mNm").arg(MAX_TORQUE));
    ui->label_Device_Status->setText(tr("Device OFF"));
    ui->label_Device_Status->setStyleSheet("QLabel { color : rgb(180,0,0); }");

    ui->label_dir_chosen->setText(tr("Choose a directory to start"));
    ui->label_dir_chosen->setStyleSheet("QLabel { color : rgb(180,0,0); }");
    ui->label_dir_chosen->setAlignment(Qt::AlignCenter);

    ui->lineEdit_SubjName->setText("Name");
    ui->lineEdit_SubjSurn->setText("Surname");

    ui->radioButton_healty->setChecked(1);
    ui->radioButton_pre->setChecked(0);
    ui->radioButton_post->setChecked(0);

    ui->torque_progressBar->setValue(0);
    ui->torque_progressBar->setEnabled(0);

    ui->spinBox_n_repetetitions->setRange(1, 255);
    ui->spinBox_n_repetetitions->setValue(5);
    ui->spinBox_n_repetetitions->setEnabled(0);

    ui->doubleSpinBox_Torque->setValue(900);
    ui->doubleSpinBox_rampDuration->setValue(3);
    ui->doubleSpinBox_positionDesired->setValue(20);

    ui->torque_progressBar->setRange(static_cast<int>(-MAX_TORQUE), static_cast<int>(MAX_TORQUE));

//    RX_BUFFER_SIZE = RXBUFFER_SIZE;

    // When data ara available readyRead function generates a trigger that start the function readData, below implemented
    connect(serial_device, &QSerialPort::readyRead, this, &MainWindow::readData);

    timer_plot = new QTimer(this); // define the timer variable to then start ciclically the timer
    connect(timer_plot, &QTimer::timeout, this, &MainWindow::Plot_Data);
    timer_plot->stop();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionExit_triggered()
{
    if (serial_device->isOpen())
    {
        serial_device->close();

        ui->label_Port_Info->setText(tr("Device Disconnected"));

        this->repaint();

        QThread::msleep(100L);

        MainWindow::on_pushButton_Stop_clicked();
    }

    close();
}

void MainWindow::on_pushButton_UpgradeList_clicked()
{
    // find and show all serial devices available
    ui->comboBox_Device_List->clear();
    static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");
    QString description;
    QString manufacturer;
    QString serialNumber;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QStringList list;

        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);
        ui->comboBox_Device_List->addItem(list.first(), list);
    }

    this->repaint();
}

void MainWindow::on_pushButton_Connect_clicked()
{
    serial_device_name = ui->comboBox_Device_List->currentText();

    baud_string = ui->comboBox_Baud_List->currentText();
    baud = baud_string.toInt();

    //---------------------- serial port settings ----------------------//
    serial_device->setPortName(serial_device_name);
    serial_device->setBaudRate(baud);
    serial_device->setDataBits(QSerialPort::Data8);
    serial_device->setParity(QSerialPort::NoParity);
    serial_device->setFlowControl(QSerialPort::NoFlowControl);
    serial_device->setStopBits(QSerialPort::OneStop);
    serial_device->open(QIODevice::ReadWrite);
    if(serial_device->isOpen())
    {
        ui->groupBox_Connection_Management->setEnabled(0);
        ui->pushButton_Connect->setEnabled(0);
        ui->pushButton_Disconnect->setEnabled(1);
        ui->groupBox_Control_Centre->setEnabled(1);
        ui->spinBox_n_repetetitions->setEnabled(1);
        ui->spinBox_n_repetetitions->setValue(5);
        ui->groupBox_PatientID->setEnabled(1);
        ui->pushButton_Stop->setEnabled(0);

        ui->radioButton_pre->setEnabled(1);
        ui->radioButton_post->setEnabled(1);
        ui->pushButton_startBM1->setEnabled(0);
        ui->pushButton_startBM2->setEnabled(0);
        ui->pushButton_Start_MM2->setEnabled(1);

        ui->label_Port_Info->setText(tr("Connected to: %1; baudrate selected: %2").arg(serial_device_name).arg(baud));
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), serial_device->errorString());
        ui->label_Port_Info->setText(tr("Cannot open %1 ").arg(serial_device_name));
    }

    this->repaint();
}

void MainWindow::on_pushButton_Disconnect_clicked()
{
    if (serial_device->isOpen())
    {
        serial_device->close();
    }


    if(device_mode != stop)
    {
        MainWindow::on_pushButton_Stop_clicked();
    }

    ui->pushButton_Connect->setEnabled(1);
    ui->groupBox_Connection_Management->setEnabled(1);
    ui->pushButton_Disconnect->setEnabled(0);
    ui->groupBox_Control_Centre->setEnabled(0);
    ui->actionChooseDirectory->setEnabled(1);
    ui->actionExit->setEnabled(1);

    ui->spinBox_n_repetetitions->setEnabled(0);
    ui->spinBox_n_repetetitions->setValue(0);

    ui->groupBox_PatientID->setEnabled(0);

    ui->label_Port_Info->setText(tr("Device Disconnected"));

    ui->doubleSpinBox_Torque->setValue(0);

    ui->torque_progressBar->setValue(0);

    ui->label_Torque_Info->setText(tr("Max torque: %1 mNm").arg(MAX_TORQUE));

    this->repaint();
}

void MainWindow::on_pushButton_Start_MM_clicked()
{
    QString subj_name;
    QString subj_surn;
    QString prova;
    QString session;
    QString speed_session;

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentDateTime_str = currentDateTime.toString("yyyy-MM-dd_HH.mm.ss");
    uint8_t n_repetitions_of_perturbation = 0;

    ui->actionChooseDirectory->setEnabled(0);
    ui->actionExit->setEnabled(0);
    ui->doubleSpinBox_Torque->setEnabled(0);
    ui->spinBox_n_repetetitions->setEnabled(0);
    ui->pushButton_Start_MM->setEnabled(0);
    ui->pushButton_Start_MM2->setEnabled(0);
    ui->pushButton_startBM1->setEnabled(0);
    ui->pushButton_startBM2->setEnabled(0);
    ui->pushButton_StartPosCtrl->setEnabled(0);
    ui->torque_progressBar->setValue(0);
    ui->pushButton_Stop->setEnabled(1);

    subj_name = ui->lineEdit_SubjName->text();
    subj_surn = ui->lineEdit_SubjSurn->text();

    QTextStream stream_out_log_file(&log_file);

    prova = currentDateTime_str + "_" + subj_surn[0] + "_" + subj_name[0] + "_positive";

    if(ui->radioButton_pre->isChecked())
    {
        session = "_PRE";
    }
    else if(ui->radioButton_post->isChecked())
    {
        session = "_POST";
    }
    else if(ui->radioButton_healty->isChecked())
    {
        session = "_HEALTHY";
    }

    u.double_torque = MainWindow::getTorque();

    // Get Ramp duration
    ramp_duration.data_double = getRampDuration();
    speed_session = QString::number(ramp_duration.data_double);

    file.setFileName(directory.path() + "/" + currentDateTime_str + "_" + subj_surn[0] + subj_name[0] + "_" + speed_session + session + "_MM1.txt");
    file.open(QIODevice::WriteOnly); // Or QIODevice::ReadWrite

    log_file.setFileName(directory.path() + "/" + currentDateTime_str + "_" + subj_surn[0] + subj_name[0] + "_" + speed_session + session + "_MM1_log.me");
    log_file.open(QIODevice::WriteOnly);
    stream_out_log_file << "Luigi Raiano, PDMeter, App: " << app_name << "." << endl;
    stream_out_log_file << "Date: " << currentDateTime_str << endl;
    stream_out_log_file << "Mode: MM1 " << endl;
    stream_out_log_file << "Subj ID: " << subj_surn << " " << subj_name << endl;
    stream_out_log_file << endl;
    stream_out_log_file << "Steady desired torque: " << u.double_torque << endl;
    stream_out_log_file << "Ramp duration: " << speed_session << endl;
    stream_out_log_file << "Sampling Rate: " << srate_uart << " [Hz]" << endl;
    stream_out_log_file << "Variable Stored:" << endl;
    stream_out_log_file << "time [s] \t V_forceL \t V_forceH \t V_currL \t V_currH \t Control Torque [mNm] \t Theta [deg] \t theta desired [deg]" << endl;
    stream_out_log_file << endl;


    QByteArray uartTXBuffer(nullptr);
    uartTXBuffer.resize(DOUBLE_DATA_SIZE+1);


//    RXBuffer = new uint8_t[RX_BUFFER_SIZE];
    //    ui->torque_progressBar->setRange(0, static_cast<int>(abs(u.double_torque)));

    QString value1 = "QProgressBar::chunk {background-color: #108d00; width: 2px}"; //#1c2162
    ui->torque_progressBar->setStyleSheet(value1);

    n_repetitions_of_perturbation = static_cast <uint8_t> (ui->spinBox_n_repetetitions->value());

    SendDataToMCU(flag_start, n_repetitions_of_perturbation, uartTXBuffer);

    device_mode = MM1;

    this->repaint();
}

void MainWindow::on_pushButton_Start_MM2_clicked()
{
    QString subj_name;
    QString subj_surn;
    QString prova;
    QString session;
    QString speed_session;

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentDateTime_str = currentDateTime.toString("yyyy-MM-dd_HH.mm.ss");
    uint8_t n_repetitions_of_perturbation = 0;

    QTextStream stream_out_log_file(&log_file);

    ui->actionChooseDirectory->setEnabled(0);
    ui->actionExit->setEnabled(0);
    ui->doubleSpinBox_Torque->setEnabled(0);
    ui->spinBox_n_repetetitions->setEnabled(0);
    ui->pushButton_Start_MM->setEnabled(0);
    ui->pushButton_Start_MM2->setEnabled(0);
    ui->pushButton_startBM1->setEnabled(0);
    ui->pushButton_startBM2->setEnabled(0);
    ui->pushButton_StartPosCtrl->setEnabled(0);
    ui->torque_progressBar->setValue(0);
    ui->pushButton_Stop->setEnabled(1);

    subj_name = ui->lineEdit_SubjName->text();
    subj_surn = ui->lineEdit_SubjSurn->text();

    if(ui->radioButton_pre->isChecked())
    {
        session = "_PRE";
    }
    else if(ui->radioButton_post->isChecked())
    {
        session = "_POST";
    }
    else if(ui->radioButton_healty->isChecked())
    {
        session = "_HEALTHY";
    }

    u.double_torque = MainWindow::getTorque();

    // Get Ramp duration
    ramp_duration.data_double = getRampDuration();
    speed_session = QString::number(ramp_duration.data_double);

    file.setFileName(directory.path() + "/" + currentDateTime_str + "_" + subj_surn[0] + subj_name[0] + "_" + speed_session + session + "_MM2.txt");
    file.open(QIODevice::WriteOnly); // Or QIODevice::ReadWrite

    log_file.setFileName(directory.path() + "/" + currentDateTime_str + + "_" + subj_surn[0]  + subj_name[0] + "_" + speed_session + session + "_MM2_log.me");
    log_file.open(QIODevice::WriteOnly);
    stream_out_log_file << "Luigi Raiano, PDMeter, App: " << app_name << "." << endl;
    stream_out_log_file << "Date: " << currentDateTime_str << endl;
    stream_out_log_file << "Mode: MM2 " << endl;
    stream_out_log_file << "Subj ID: " << subj_surn << " " << subj_name << endl;
    stream_out_log_file << "Steady desired torque: " << u.double_torque << endl;
    stream_out_log_file << "Ramp duration: " << speed_session << endl;
    stream_out_log_file << "Sampling Rate: " << srate_uart << " [Hz]" << endl;
    stream_out_log_file << endl;
    stream_out_log_file << "Variable Stored:" << endl;
    stream_out_log_file << "time [s] \t V_forceL \t V_forceH \t V_currL \t V_currH \t Control Torque [mNm] \t Theta [deg] \t theta desired [deg]" << endl;
    stream_out_log_file << endl;

    QByteArray uartTXBuffer(nullptr);
    uartTXBuffer.resize(DOUBLE_DATA_SIZE+1);

    QString value1 = "QProgressBar::chunk {background-color: #1c2162; width: 2px}";
    ui->torque_progressBar->setStyleSheet(value1);

    n_repetitions_of_perturbation = static_cast <uint8_t> (ui->spinBox_n_repetetitions->value());

    SendDataToMCU(flag_start2, n_repetitions_of_perturbation, uartTXBuffer);

    device_mode = MM2;

    this->repaint();
}

void MainWindow::on_pushButton_startBM1_clicked()
{
    QString subj_name;
    QString subj_surn;
    QString prova;
    QString session;

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentDateTime_str = currentDateTime.toString("yyyy-MM-dd_HH.mm.ss");
    uint8_t n_repetitions_of_perturbation = 0;

    QTextStream stream_out_log_file(&log_file);

    ui->actionChooseDirectory->setEnabled(0);
    ui->actionExit->setEnabled(0);
    ui->doubleSpinBox_Torque->setEnabled(0);
    ui->spinBox_n_repetetitions->setEnabled(0);
    ui->pushButton_Start_MM->setEnabled(0);
    ui->pushButton_Start_MM2->setEnabled(0);
    ui->pushButton_startBM1->setEnabled(0);
    ui->pushButton_startBM2->setEnabled(0);
    ui->pushButton_StartPosCtrl->setEnabled(0);
    ui->torque_progressBar->setValue(0);
    ui->pushButton_Stop->setEnabled(1);

    subj_name = ui->lineEdit_SubjName->text();
    subj_surn = ui->lineEdit_SubjSurn->text();

    if(ui->radioButton_pre->isChecked())
    {
        session = "_PRE";
    }
    else if(ui->radioButton_post->isChecked())
    {
        session = "_POST";
    }
    else if(ui->radioButton_healty->isChecked())
    {
        session = "_HEALTHY";
    }

    file.setFileName(directory.path() + "/" + currentDateTime_str + "_" + subj_surn[0] + subj_name[0] + session + "_BM1.txt");
    file.open(QIODevice::WriteOnly); // Or QIODevice::ReadWrite

    log_file.setFileName(directory.path() + "/" + currentDateTime_str + + "_" + subj_surn[0] + subj_name[0] + session + "_BM1_log.me");
    log_file.open(QIODevice::WriteOnly);
    stream_out_log_file << "Luigi Raiano, PDMeter, App: " << app_name << "." << endl;
    stream_out_log_file << "Date: " << currentDateTime_str << endl;
    stream_out_log_file << "Mode: BM1 " << endl;
    stream_out_log_file << "Subj ID: " << subj_surn << " " << subj_name << endl;
    stream_out_log_file << "Sampling Rate: " << srate_uart << " [Hz]" << endl;
    stream_out_log_file << endl;
    stream_out_log_file << "Variable Stored:" << endl;
    stream_out_log_file << "time [s] \t V_forceL \t V_forceH \t V_currL \t V_currH \t Control Torque [mNm] \t Theta [deg] \t theta desired [deg]" << endl;
    stream_out_log_file << endl;

    QByteArray uartTXBuffer(nullptr);
    uartTXBuffer.resize(DOUBLE_DATA_SIZE+1);

    u.double_torque = 0;
    // Get Ramp duration
    ramp_duration.data_double = getRampDuration();

    n_repetitions_of_perturbation = static_cast <uint8_t> (ui->spinBox_n_repetetitions->value());

//    RXBuffer = new uint8_t[RX_BUFFER_SIZE];

    SendDataToMCU(flag_startBM1, n_repetitions_of_perturbation, uartTXBuffer);

    device_mode = BM1;

    this->repaint();
}

void MainWindow::on_pushButton_startBM2_clicked()
{
    QString subj_name;
    QString subj_surn;
    QString prova;
    QString session;

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentDateTime_str = currentDateTime.toString("yyyy-MM-dd_HH.mm.ss");
    uint8_t n_repetitions_of_perturbation = 0;

    QTextStream stream_out_log_file(&log_file);

    ui->actionChooseDirectory->setEnabled(0);
    ui->actionExit->setEnabled(0);
    ui->doubleSpinBox_Torque->setEnabled(0);
    ui->spinBox_n_repetetitions->setEnabled(0);
    ui->pushButton_Start_MM->setEnabled(0);
    ui->pushButton_Start_MM2->setEnabled(0);
    ui->pushButton_startBM1->setEnabled(0);
    ui->pushButton_startBM2->setEnabled(0);
    ui->pushButton_StartPosCtrl->setEnabled(0);
    ui->torque_progressBar->setValue(0);
    ui->pushButton_Stop->setEnabled(1);

    subj_name = ui->lineEdit_SubjName->text();
    subj_surn = ui->lineEdit_SubjSurn->text();

    if(ui->radioButton_pre->isChecked())
    {
        session = "_PRE";
    }
    else if(ui->radioButton_post->isChecked())
    {
        session = "_POST";
    }
    else if(ui->radioButton_healty->isChecked())
    {
        session = "_HEALTHY";
    }

    file.setFileName(directory.path() + "/" + currentDateTime_str + "_" + subj_surn[0] + subj_name[0] + session + "_BM2.txt");
    file.open(QIODevice::WriteOnly); // Or QIODevice::ReadWrite

    log_file.setFileName(directory.path() + "/" + currentDateTime_str + + "_" + subj_surn[0] + subj_name[0]  + session + "_BM2_log.me");
    log_file.open(QIODevice::WriteOnly);
    stream_out_log_file << "Luigi Raiano, PDMeter, App: " << app_name << "." << endl;
    stream_out_log_file << "Date: " << currentDateTime_str << endl;
    stream_out_log_file << "Mode: BM2 " << endl;
    stream_out_log_file << "Subj ID: " << subj_surn << " " << subj_name << endl;
    stream_out_log_file << "Sampling Rate: " << srate_uart << " [Hz]" << endl;
    stream_out_log_file << endl;
    stream_out_log_file << "Variable Stored:" << endl;
    stream_out_log_file << "time [s] \t V_forceL \t V_forceH \t V_currL \t V_currH \t Control Torque [mNm] \t Theta [deg] \t theta desired [deg]" << endl;
    stream_out_log_file << endl;

    QByteArray uartTXBuffer(nullptr);
    uartTXBuffer.resize(DOUBLE_DATA_SIZE+1);

    u.double_torque = 0;
    // Get Ramp duration
    ramp_duration.data_double = getRampDuration();

    n_repetitions_of_perturbation = static_cast <uint8_t> (ui->spinBox_n_repetetitions->value());

//    RXBuffer = new uint8_t[RX_BUFFER_SIZE];

    SendDataToMCU(flag_startBM2, n_repetitions_of_perturbation, uartTXBuffer);

    device_mode = BM2;

    this->repaint();
}

void MainWindow::on_pushButton_StartPosCtrl_clicked()
{
    QString subj_name;
    QString subj_surn;
    QString prova;
    QString session;
    QString speed_session;

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString currentDateTime_str = currentDateTime.toString("yyyy-MM-dd_HH.mm.ss");
    uint8_t n_repetitions_of_perturbation = 0;

    QTextStream stream_out_log_file(&log_file);

    ui->actionChooseDirectory->setEnabled(0);
    ui->actionExit->setEnabled(0);
    ui->doubleSpinBox_Torque->setEnabled(0);
    ui->spinBox_n_repetetitions->setEnabled(0);
    ui->pushButton_Start_MM->setEnabled(0);
    ui->pushButton_Start_MM2->setEnabled(0);
    ui->pushButton_startBM1->setEnabled(0);
    ui->pushButton_startBM2->setEnabled(0);
    ui->pushButton_StartPosCtrl->setEnabled(0);
    ui->torque_progressBar->setValue(0);
    ui->pushButton_Stop->setEnabled(1);

    subj_name = ui->lineEdit_SubjName->text();
    subj_surn = ui->lineEdit_SubjSurn->text();

    if(ui->radioButton_pre->isChecked())
    {
        session = "_PRE";
    }
    else if(ui->radioButton_post->isChecked())
    {
        session = "_POST";
    }
    else if(ui->radioButton_healty->isChecked())
    {
        session = "_HEALTHY";
    }

    // Assign data - torque
    theta_d.data_double = getThetaDesired();
    // Get Ramp duration
    ramp_duration.data_double = getRampDuration();
    speed_session = QString::number(ramp_duration.data_double);

    file.setFileName(directory.path() + "/" + currentDateTime_str + "_" + subj_surn[0] + subj_name[0] + "_" + speed_session + session + "_PC1.txt");
    file.open(QIODevice::WriteOnly); // Or QIODevice::ReadWrite

    log_file.setFileName(directory.path() + "/" + currentDateTime_str + + "_" + subj_surn[0] + subj_name[0] + "_" + speed_session + session + "_PC1_log.me");
    log_file.open(QIODevice::WriteOnly);
    stream_out_log_file << "Luigi Raiano, PDMeter, App: " << app_name << "." << endl;
    stream_out_log_file << "Date: " << currentDateTime_str << endl;
    stream_out_log_file << "Mode: Position Controller 1 " << endl;
    stream_out_log_file << "Subj ID: " << subj_surn << " " << subj_name << endl;
    stream_out_log_file << "Steady desired position: " << theta_d.data_double << endl;
    stream_out_log_file << "Ramp duration: " << speed_session << endl;
    stream_out_log_file << "Sampling Rate: " << srate_uart << " [Hz]" << endl;
    stream_out_log_file << endl;
    stream_out_log_file << "Variable Stored:" << endl;
    stream_out_log_file << "time [s] \t V_forceL \t V_forceH \t V_currL \t V_currH \t Control Torque [mNm] \t Theta [deg] \t theta desired [deg]" << endl;
    stream_out_log_file << endl;

    QByteArray uartTXBuffer(nullptr);
    uartTXBuffer.resize(DOUBLE_DATA_SIZE+1);

    n_repetitions_of_perturbation = static_cast <uint8_t> (ui->spinBox_n_repetetitions->value());

//    RXBuffer = new uint8_t[RX_BUFFER_SIZE];

    SendDataToMCU(flag_startPC1, n_repetitions_of_perturbation, uartTXBuffer);

    device_mode = PC1;

    this->repaint();
}

void MainWindow::on_pushButton_Stop_clicked()
{
    QByteArray uartTXBuffer(nullptr);
    uartTXBuffer.resize(DOUBLE_DATA_SIZE+1);
    QTextStream stream_out_file_rejected(&log_file);

    ui->actionChooseDirectory->setEnabled(1);
    ui->actionExit->setEnabled(1);
    ui->doubleSpinBox_Torque->setEnabled(1);
    ui->spinBox_n_repetetitions->setEnabled(1);
    ui->pushButton_Start_MM->setEnabled(1);
    ui->pushButton_Start_MM2->setEnabled(1);
    ui->pushButton_startBM1->setEnabled(0);
    ui->pushButton_startBM2->setEnabled(0);
    ui->pushButton_StartPosCtrl->setEnabled(1);
    ui->pushButton_Stop->setEnabled(0);
    ui->torque_progressBar->setValue(0);

    stream_out_file_rejected << "Rejected data: " << rejected;
    rejected = 0;

//    delete[] RXBuffer;

    SendDataToMCU(flag_stop, 0, uartTXBuffer);

    device_mode = stop;

    uartRXBuffer_append.clear();

    time = 0;

    if(file.isOpen())
    {
        file.close();
    }

    if(log_file.isOpen())
    {
        log_file.close();
    }

    this->repaint();
}

void MainWindow::readData()
{
    // Function connected to trigger signal QSerailPrt::readyRead();

    QByteArray uartRXBuffer(nullptr);

    QByteArray uartRXBuffer_tmp(nullptr);

    QTextStream stream_out_file(&file);

    uint8_t RXBuffer[RX_BUFFER_SIZE];

    uartRXBuffer.append(uartRXBuffer_append);
    uartRXBuffer_tmp = serial_device->readAll();

    uartRXBuffer.append(uartRXBuffer_tmp); // read all available data

    int i = 0;
    while(i<(uartRXBuffer.length() - RX_BUFFER_SIZE))
    {
        if(UART_PRE == uartRXBuffer[i] && (UART_DEV_ID == uartRXBuffer[i+1]))
        {
            // Read all data
            int k = 0;
            for(int j=i; j<(i+RX_BUFFER_SIZE); j++){
                RXBuffer[k] = static_cast<uint8_t>(uartRXBuffer[j]);
                k++;
            }

            // get time
            k = 2;
            for(int j=0; j<4; j++)
            {
                time_stmp.data_uint8[j] = RXBuffer[k];
                k++;
            }

            time = time_stmp.data_uint32/sample_freq_time;

            // Read Force
            for(int j=0; j<2; j++)
            {
                force_measured[j] = RXBuffer[k];
                k++;
            }

            // Read Current
            for(int j=0; j<2; j++)
            {
                current_measured[j] = RXBuffer[k];
                k++;
            }

            // tau desired
            for(int i=0; i<DOUBLE_DATA_SIZE; i++)
            {
                tau_d.twos_complement_array_torque_desired[i] = RXBuffer[k];
                k++;
            }

            // Theta measured
            for(int i=0; i<DOUBLE_DATA_SIZE; i++){
                theta.theta_uint8[i] = RXBuffer[k];
                k++;
            }

            //Theta error
            for(int i=0; i<DOUBLE_DATA_SIZE; i++){
                theta_e.data_uint8[i] = RXBuffer[k];
                k++;
            }

            // write time
            stream_out_file << time << "\t";

            // Write force
            stream_out_file << force_measured[0] << "\t"; // H
            stream_out_file << force_measured[1] << "\t"; // L

            // Write current
            stream_out_file << current_measured[0] << "\t"; // H
            stream_out_file << current_measured[1] << "\t"; // L

            stream_out_file << tau_d.torque_desired << "\t"; // L

            stream_out_file << theta.theta_double << "\t";

            stream_out_file << theta_e.data_double << endl;

            i += RX_BUFFER_SIZE;
        } // end if
        else
        {
            i++;
        }
    }// end while i

    /**** Keep data not stored for the next loop *****/
    idx = (uartRXBuffer.length()-i);

    uartRXBuffer_append.resize(idx);
    for(int h=0; h<idx; h++)
    {
        uartRXBuffer_append[h] = uartRXBuffer[i];
        i++;
    }
    /**** Keep data not stored for the next loop *****/

    this->repaint();
}


void MainWindow::on_actionChooseDirectory_triggered()
{
    if(!dir_chosen){
        dir_path = QFileDialog::getExistingDirectory (this, tr("Choose a directory to store data"), directory.path());
        if ( dir_path.isNull() == false )
        {

            if (serial_device->isOpen())
            {
            }
            else
            {
                directory.setPath(dir_path);
                ui->groupBox_Connection_Management->setEnabled(1);
                ui->groupBox_Pairing->setEnabled(1);
                ui->pushButton_Disconnect->setEnabled(0);
                ui->groupBox_Control_Centre->setEnabled(0);
                ui->torque_progressBar->setEnabled(1);

            }

            QString dir_chosen = QString("Chosen directory: ") + (directory.path());
            ui->label_dir_chosen->setText(dir_chosen);
            QFont font("NS Text", 10, QFont::Bold);
            ui->label_dir_chosen->setFont(font);
            ui->label_dir_chosen->setStyleSheet("QLabel { color : rgb(0,0,0); }");
            ui->label_dir_chosen->setAlignment(Qt::AlignLeft);

            dir_chosen = true;

            this->repaint();
        }
    }
}

void MainWindow::SendDataToMCU(uint8_t Command, uint8_t n_perturbations_provided,QByteArray &uartTXBuffer)
{
    if(Command == flag_start)
    {
        // Assign data
        // Torque desired
        int k=0;
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(u.twos_complement_array_torque[i]);
            k++;
        }

        // Ramp duration
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(ramp_duration.data_uint8[i]);
            k++;
        }

        // Number of pertubation to be applied
        uartTXBuffer[UART_TX_DATA-2] = static_cast<char>(n_perturbations_provided);

        // Assign flag
        uartTXBuffer[UART_TX_DATA-1] = static_cast<char>(Command);

        // Send data over uart
        serial_device->write(uartTXBuffer);

        // Start timer for plotting data
        timer_plot->start(plot_refresh_period); // interrupt flug each every "period_IT_timer" ms

        QString torque_str = QString("%1 mNm").arg(u.double_torque);

        // Upgrade infos
        ui->label_Device_Status->setText(tr("Device ON"));
        ui->label_Device_Status->setStyleSheet("QLabel { color : rgb(0,105,0); }");

        Start = true;
    }
    else if(Command == flag_start2)
    {
        // Torque desired
        int k=0;
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(u.twos_complement_array_torque[i]);
            k++;
        }

        // Ramp duration
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(ramp_duration.data_uint8[i]);
            k++;
        }

        // Number of pertubation to be applied
        uartTXBuffer[UART_TX_DATA-2] = static_cast<char>(n_perturbations_provided);

        // Assign flag
        uartTXBuffer[UART_TX_DATA-1] = static_cast<char>(Command);

        // Send data over uart
        serial_device->write(uartTXBuffer);

        // Start timer for plotting data
        timer_plot->start(plot_refresh_period); // interrupt flug each every "period_IT_timer" ms

        QString torque_str = QString("%1 mNm").arg(u.double_torque);

        // Upgrade infos
        ui->label_Device_Status->setText(tr("Device ON"));
        ui->label_Device_Status->setStyleSheet("QLabel { color : rgb(0,105,0); }");
        Start = true;
    }
    else if(Command == flag_startBM1)
    {
        // Torque desired
        int k=0;
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(u.twos_complement_array_torque[i]);
            k++;
        }

        // Ramp duration
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(ramp_duration.data_uint8[i]);
            k++;
        }

        // Number of pertubation to be applied
        uartTXBuffer[UART_TX_DATA-2] = static_cast<char>(n_perturbations_provided);

        // Assign flag
        uartTXBuffer[UART_TX_DATA-1] = static_cast<char>(Command);

        // Send data over uart
        serial_device->write(uartTXBuffer);

        // Start timer for plotting data
        timer_plot->start(plot_refresh_period); // interrupt flug each every "period_IT_timer" ms

        QString torque_str = "BM1 on";

        // Upgrade infos
        ui->label_Device_Status->setText(tr("Device ON"));
        ui->label_Device_Status->setStyleSheet("QLabel { color : rgb(0,105,0); }");
        Start = true;
    }
    else if(Command == flag_startBM2)
    {
        // Torque desired
        int k=0;
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(u.twos_complement_array_torque[i]);
            k++;
        }

        // Ramp duration
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(ramp_duration.data_uint8[i]);
            k++;
        }

        // Number of pertubation to be applied
        uartTXBuffer[UART_TX_DATA-2] = static_cast<char>(n_perturbations_provided);

        // Assign flag
        uartTXBuffer[UART_TX_DATA-1] = static_cast<char>(Command);

        // Send data over uart
        serial_device->write(uartTXBuffer);

        // Start timer for plotting data
        timer_plot->start(plot_refresh_period); // interrupt flug each every "period_IT_timer" ms

        QString torque_str = "BM2 on";

        // Upgrade infos
        ui->label_Device_Status->setText(tr("Device ON"));
        ui->label_Device_Status->setStyleSheet("QLabel { color : rgb(0,105,0); }");
        Start = true;
    }
    else if(Command == flag_startPC1)
    {
        // Torque desired
        int k=0;
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(theta_d.data_uint8[i]);
            k++;
        }

        // Ramp duration
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(ramp_duration.data_uint8[i]);
            k++;
        }

        // Number of pertubation to be applied
        uartTXBuffer[UART_TX_DATA-2] = static_cast<char>(n_perturbations_provided);

        // Assign flag
        uartTXBuffer[UART_TX_DATA-1] = static_cast<char>(Command);

        // Send data over uart
        serial_device->write(uartTXBuffer);

        // Start timer for plotting data
        timer_plot->start(plot_refresh_period); // interrupt flug each every "period_IT_timer" ms

        QString torque_str = "PC1 on";

        // Upgrade infos
        ui->label_Device_Status->setText(tr("Device ON"));
        ui->label_Device_Status->setStyleSheet("QLabel { color : rgb(0,105,0); }");
        Start = true;
    }
    else if(Command == flag_stop)
    {
        // Torque desired
        int k=0;
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(theta_d.data_uint8[i]);
            k++;
        }

        // Ramp duration
        for(int i=0; i<(DOUBLE_DATA_SIZE);i++){
            uartTXBuffer[k] = static_cast<char>(ramp_duration.data_uint8[i]);
            k++;
        }

        // Number of pertubation to be applied
        uartTXBuffer[UART_TX_DATA-2] = static_cast<char>(n_perturbations_provided);

        // Assign flag
        uartTXBuffer[UART_TX_DATA-1] = static_cast<char>(Command);

        // Send data over uart
        serial_device->write(uartTXBuffer);

        // Start timer for plotting data
        timer_plot->stop(); // interrupt flug each every "period_IT_timer" ms

        // Upgrade infos
        ui->label_Torque_Info->setText(tr("Max torque: %1 mNm").arg(MAX_TORQUE));
        ui->label_Device_Status->setText(tr("Device OFF"));
        ui->label_Device_Status->setStyleSheet("QLabel { color : rgb(180,0,0); }");

        Start = false;

        file.close();
        log_file.close();
    }
}

void MainWindow::on_radioButton_pre_clicked()
{
    ui->radioButton_post->setChecked(0);
    ui->radioButton_healty->setChecked(0);
}

void MainWindow::on_radioButton_post_clicked()
{
    ui->radioButton_pre->setChecked(0);
    ui->radioButton_healty->setChecked(0);
}

void MainWindow::on_radioButton_healty_clicked()
{
    ui->radioButton_pre->setChecked(0);
    ui->radioButton_post->setChecked(0);
}

void MainWindow::Plot_Data()
{
    double y;

    switch(device_mode)
    {

    case MM1:
    {
        y = static_cast<double>(tau_d.torque_desired);
        // plot the control torque in the lifting bar
        ui->torque_progressBar->setValue(static_cast<int>(y));
    }break;

    case MM2:
    {
        y = static_cast<double>(tau_d.torque_desired);
        // plot the control torque in the lifting bar
        ui->torque_progressBar->setValue(static_cast<int>(y));
    }break;

    case MM3:
    {
        y = static_cast<double>(tau_d.torque_desired);
        // plot the control torque in the lifting bar
        ui->torque_progressBar->setValue(static_cast<int>(y));
    }break;

    case PC1:
    {
        y = static_cast<double>(theta.theta_double);
        // plot the control torque in the lifting bar
        ui->torque_progressBar->setValue(static_cast<int>(y*100));
    }break;

    } // end switch

    this->repaint();
}

double MainWindow::getTorque()
{
    double tau;
    tau = ui->doubleSpinBox_Torque->value(); // joint torque [mNm]

    return tau;
}

double MainWindow::getThetaDesired()
{
    double theta_des = 0;
    return theta_des = ui->doubleSpinBox_positionDesired->value();
}

double MainWindow::getRampDuration()
{
    double ramp_duration = 0;
    return ramp_duration = ui->doubleSpinBox_rampDuration->value();
}

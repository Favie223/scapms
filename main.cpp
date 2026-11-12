
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;

    window.setWindowTitle("Smart Clinic System");
    window.resize(800, 550);

    QVBoxLayout *layout = new QVBoxLayout(&window);

    QLabel *title = new QLabel("SMART CLINIC SYSTEM");

    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont("Arial", 22, QFont::Bold));

    layout->addWidget(title);

    QLabel *subtitle = new QLabel(
        "Welcome to your Clinic Management Dashboard"
    );

    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    QPushButton *registerBtn =
        new QPushButton("Register Patient");

    QPushButton *recordsBtn =
        new QPushButton("View Patient Records");

    QPushButton *appointmentBtn =
        new QPushButton("Book Appointment");

    QPushButton *doctorBtn =
        new QPushButton("Doctor Information");

    QPushButton *exitBtn =
        new QPushButton("Exit");

    layout->addWidget(registerBtn);
    layout->addWidget(recordsBtn);
    layout->addWidget(appointmentBtn);
    layout->addWidget(doctorBtn);
    layout->addWidget(exitBtn);

    QObject::connect(registerBtn, &QPushButton::clicked,
        [&]() {
            QMessageBox::information(
                &window, "Patient Registration",
                "Patient Registration Selected"
            );
        });

    QObject::connect(recordsBtn, &QPushButton::clicked,
        [&]() {
            QMessageBox::information(
                &window, "Patient Records",
                "Patient Records Selected"
            );
        });

    QObject::connect(appointmentBtn, &QPushButton::clicked,
        [&]() {
            QMessageBox::information(
                &window, "Appointments",
                "Appointment Booking Selected"
            );
        });

    QObject::connect(doctorBtn, &QPushButton::clicked,
        [&]() {
            QMessageBox::information(
                &window, "Doctors",
                "Doctor Information Selected"
            );
        });

    QObject::connect(exitBtn, &QPushButton::clicked,
        &app, &QApplication::quit);

    window.show();

    return app.exec();
}

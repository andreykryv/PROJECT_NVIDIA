#include "aboutdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QFont>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , logoAnimation(nullptr)
{
    setWindowTitle("О программе");
    setFixedSize(500, 400);
    
    setupUi();
    populateInfo();
    startLogoAnimation();
}

void AboutDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Логотип
    logoLabel = new QLabel(this);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setFixedSize(80, 80);
    logoLabel->setStyleSheet(
        "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "stop:0 #3498DB, stop:1 #2ECC71);"
        "border-radius: 40px;"
        "font-size: 36px; font-weight: bold; color: white;"
    );
    logoLabel->setText("SB");
    mainLayout->addWidget(logoLabel);
    
    // Заголовок
    titleLabel = new QLabel("SortBench CUDA vs CPU", this);
    QFont titleFont("Arial", 18, QFont::Bold);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Версия
    versionLabel = new QLabel("v1.0.0", this);
    versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(versionLabel);
    
    // Описание
    descriptionLabel = new QLabel(
        "Приложение для бенчмаркинга и визуализации алгоритмов сортировки.\n"
        "Сравнение производительности CPU и GPU реализаций.", this);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(descriptionLabel);
    
    // Технологии
    techLabel = new QLabel(this);
    techLabel->setWordWrap(true);
    mainLayout->addWidget(techLabel);
    
    // CUDA информация
    cudaInfoLabel = new QLabel(this);
    cudaInfoLabel->setWordWrap(true);
    mainLayout->addWidget(cudaInfoLabel);
    
    // Лицензия
    licenseLabel = new QLabel("Лицензия: MIT", this);
    licenseLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(licenseLabel);
    
    mainLayout->addStretch();
    
    // Кнопки
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    
    githubBtn = new QPushButton("GitHub", this);
    githubBtn->setStyleSheet("background-color: #333; color: white; padding: 8px 16px;");
    btnLayout->addWidget(githubBtn);
    
    btnLayout->addStretch();
    
    closeBtn = new QPushButton("Закрыть", this);
    closeBtn->setDefault(true);
    btnLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(btnLayout);
    
    connect(closeBtn, &QPushButton::clicked, this, &AboutDialog::accept);
    connect(githubBtn, &QPushButton::clicked, this, &AboutDialog::onGithubButtonClicked);
}

void AboutDialog::populateInfo() {
    QString version = QApplication::applicationVersion();
    versionLabel->setText(QString("v%1").arg(version.isEmpty() ? "1.0.0" : version));
    
    techLabel->setText(
        "Технологии:\n"
        "• Qt 6.x\n"
        "• CUDA Toolkit\n"
        "• Thrust Library\n"
        "• C++20\n"
        "• Qt Charts"
    );
    
    // CUDA информация (заглушка, если нет устройства)
    cudaInfoLabel->setText(
        "CUDA:\n"
        "Статус: Проверка устройств...\n"
        "GPU: Не определено"
    );
}

void AboutDialog::startLogoAnimation() {
    logoAnimation = new QPropertyAnimation(logoLabel, "geometry", this);
    
    QRect startRect = logoLabel->geometry();
    QRect endRect = startRect.translated(5, 5);
    
    logoAnimation->setDuration(1000);
    logoAnimation->setStartValue(startRect);
    logoAnimation->setEndValue(endRect);
    logoAnimation->setLoopCount(-1); // Бесконечный цикл
    logoAnimation->setDirection(QAbstractAnimation::Forward);
    logoAnimation->start();
}

void AboutDialog::onGithubButtonClicked() {
    QDesktopServices::openUrl(QUrl("https://github.com/cuda-lab/sortbench"));
}

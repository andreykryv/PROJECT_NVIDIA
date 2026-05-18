#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() override = default;

private slots:
    void onGithubButtonClicked();

private:
    void setupUi();
    void populateInfo();
    void startLogoAnimation();

    QLabel *logoLabel;
    QLabel *titleLabel;
    QLabel *versionLabel;
    QLabel *descriptionLabel;
    QLabel *techLabel;
    QLabel *cudaInfoLabel;
    QLabel *licenseLabel;
    QPushButton *closeBtn;
    QPushButton *githubBtn;
    
    QPropertyAnimation *logoAnimation;
};

#endif // ABOUTDIALOG_H

#pragma once

#include "BaseWizardPage.h"

class QLabel;
class QPushButton;

/**
 * "Connect your Minecraft account": why a Microsoft account is needed, the button
 * that opens Microsoft's sign-in, and "Skip for now", which asks once before it
 * lets the player in without one.
 */
class LoginWizardPage : public BaseWizardPage {
    Q_OBJECT

   public:
    explicit LoginWizardPage(QWidget* parent = nullptr);
    ~LoginWizardPage() override = default;

    void initializePage() override;
    bool validatePage() override;
    bool ownsButtons() override { return true; }
    void retranslate() override;

   private:
    void signIn();
    void skip();
    void finish();

    QLabel* m_step;
    QLabel* m_title;
    QLabel* m_body;
    QLabel* m_note;
    QPushButton* m_signIn;
    QPushButton* m_skip;
};

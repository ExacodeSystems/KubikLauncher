#pragma once

#include "BaseWizardPage.h"

class KubikLanguagePicker;
class QLabel;

/** The first question: which language. Two flag cards; choosing one switches everything at once. */
class LanguageWizardPage : public BaseWizardPage {
    Q_OBJECT
   public:
    explicit LanguageWizardPage(QWidget* parent = nullptr);

    virtual ~LanguageWizardPage();

    void initializePage() override;

    bool validatePage() override;

   protected:
    void retranslate() override;

   private:
    QLabel* m_step = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_body = nullptr;
    KubikLanguagePicker* m_picker = nullptr;
};

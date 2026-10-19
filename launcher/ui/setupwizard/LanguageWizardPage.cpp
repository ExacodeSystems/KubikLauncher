#include "LanguageWizardPage.h"
#include <Application.h>
#include "settings/SettingsObject.h"

#include <QLabel>
#include <QVBoxLayout>
#include "ui/kubik/KubikLanguagePicker.h"

LanguageWizardPage::LanguageWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    setObjectName(QStringLiteral("languagePage"));

    m_step = new QLabel(this);
    m_step->setObjectName("kubikStepLabel");
    m_title = new QLabel(this);
    m_title->setObjectName("kubikPageTitle");
    m_title->setWordWrap(true);
    m_body = new QLabel(this);
    m_body->setObjectName("kubikPageBody");
    m_body->setWordWrap(true);
    m_picker = new KubikLanguagePicker(false, this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 0);
    layout->setSpacing(8);
    layout->addWidget(m_step);
    layout->addWidget(m_title);
    layout->addWidget(m_body);
    layout->addSpacing(18);
    layout->addWidget(m_picker);
    layout->addStretch();

    retranslate();
}

LanguageWizardPage::~LanguageWizardPage() {}

void LanguageWizardPage::initializePage()
{
    m_step->setText(stepText());
    m_step->setVisible(!m_step->text().isEmpty());
}

bool LanguageWizardPage::validatePage()
{
    APPLICATION->settings()->set("Language", m_picker->selectedLanguage());
    return true;
}

void LanguageWizardPage::retranslate()
{
    m_step->setText(stepText());
    m_title->setText(tr("Choose your language"));
    m_body->setText(tr("Everything in KubikLauncher follows it. You can change it later in Settings."));
}

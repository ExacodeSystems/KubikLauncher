#include "LoginWizardPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "settings/SettingsObject.h"
#include "ui/kubik/KubikAccounts.h"
#include "ui/kubik/KubikConfirmDialog.h"
#include "ui/kubik/KubikTitle.h"

LoginWizardPage::LoginWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    setObjectName(QStringLiteral("loginPage"));

    // "Step 2 of 2", led by the eyebrow's block.
    m_step = new KubikEyebrow(this);
    m_step->setObjectName("kubikStepLabel");
    m_title = new QLabel(this);
    m_title->setObjectName("kubikPageTitle");
    m_title->setWordWrap(true);
    m_body = new QLabel(this);
    m_body->setObjectName("kubikPageBody");
    m_body->setWordWrap(true);
    m_note = new QLabel(this);
    m_note->setObjectName("kubikPageNote");
    m_note->setWordWrap(true);

    m_signIn = new QPushButton(this);
    m_signIn->setObjectName("kubikHeroPlay");
    m_signIn->setProperty("kubikRole", "primary");
    m_signIn->setIcon(KubikAccounts::microsoftIcon());
    m_signIn->setIconSize(QSize(20, 16));
    m_signIn->setCursor(Qt::PointingHandCursor);
    connect(m_signIn, &QPushButton::clicked, this, &LoginWizardPage::signIn);

    m_skip = new QPushButton(this);
    m_skip->setObjectName("kubikSkip");
    m_skip->setProperty("kubikRole", "quiet");
    m_skip->setCursor(Qt::PointingHandCursor);
    connect(m_skip, &QPushButton::clicked, this, &LoginWizardPage::skip);

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addWidget(m_signIn);
    buttons->addWidget(m_skip);
    buttons->addStretch();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 0);
    layout->setSpacing(8);
    layout->addWidget(m_step);
    layout->addWidget(m_title);
    layout->addWidget(m_body);
    layout->addSpacing(22);
    layout->addLayout(buttons);
    layout->addSpacing(14);
    layout->addWidget(m_note);
    layout->addStretch();

    retranslate();
}

void LoginWizardPage::initializePage()
{
    m_step->setText(stepText());
    m_step->setVisible(!m_step->text().isEmpty());
}

bool LoginWizardPage::validatePage()
{
    return true;
}

void LoginWizardPage::retranslate()
{
    m_step->setText(stepText());
    m_title->setText(tr("Connect your Minecraft account"));
    m_body->setText(
        tr("Minecraft needs the Microsoft account you bought it with to join servers. You sign in on Microsoft's own page, so "
           "KubikLauncher never sees your password."));
    m_note->setText(tr("You stay signed in. More accounts, or an offline profile, can be added later from your profile card."));
    m_signIn->setText(tr("Sign in with Microsoft"));
    m_skip->setText(tr("Skip for now"));
}

void LoginWizardPage::signIn()
{
    wizard()->hide();
    const auto account = KubikAccounts::addMicrosoft(nullptr);
    wizard()->show();
    if (account) {
        finish();
    }
}

void LoginWizardPage::skip()
{
    const auto answer = KubikConfirmDialog::ask(
        this, tr("Skip signing in?"),
        tr("You can browse your servers and install what they need, but you can't join one until you add a Microsoft "
           "account. You can add one any time from your profile card."),
        tr("Sign in now"), tr("Skip anyway"));
    if (answer == KubikConfirmDialog::Answer::Primary) {
        signIn();
    } else if (answer == KubikConfirmDialog::Answer::Secondary) {
        // Remembered, so the wizard does not ask again on every start; the profile card and Play still do.
        APPLICATION->settings()->set("KubikSkippedMinecraftSignIn", true);
        finish();
    }
}

void LoginWizardPage::finish()
{
    if (wizard()->currentId() == wizard()->pageIds().last()) {
        wizard()->accept();
    } else {
        wizard()->next();
    }
}

#include "SetupWizard.h"

#include "JavaWizardPage.h"
#include "LanguageWizardPage.h"

#include <Application.h>
#include <FileSystem.h>
#include "translations/TranslationsModel.h"

#include <BuildConfig.h>
#include <QAbstractButton>

#include <typeinfo>

SetupWizard::SetupWizard(QWidget* parent) : QWizard(parent)
{
    setObjectName(QStringLiteral("SetupWizard"));
    resize(640, 480);
    setMinimumSize(560, 420);
    // Classic draws the least of its own: KubikLauncher's pages bring their own titles, and a page
    // without a title gets no wizard header at all.
    setWizardStyle(QWizard::ClassicStyle);
    setOptions(QWizard::NoCancelButton | QWizard::IndependentPages | QWizard::HaveCustomButton1);

    retranslate();

    connect(this, &QWizard::currentIdChanged, this, &SetupWizard::pageChanged);
}

void SetupWizard::retranslate()
{
    setButtonText(QWizard::NextButton, tr("Continue"));
    setButtonText(QWizard::BackButton, tr("Back"));
    setButtonText(QWizard::FinishButton, tr("Continue"));
    setButtonText(QWizard::CustomButton1, tr("&Refresh"));
    setWindowTitle(tr("Set up %1").arg(BuildConfig.LAUNCHER_DISPLAYNAME));
}

BaseWizardPage* SetupWizard::getBasePage(int id)
{
    if (id == -1)
        return nullptr;
    auto pagePtr = page(id);
    if (!pagePtr)
        return nullptr;
    return dynamic_cast<BaseWizardPage*>(pagePtr);
}

BaseWizardPage* SetupWizard::getCurrentBasePage()
{
    return getBasePage(currentId());
}

void SetupWizard::pageChanged(int id)
{
    auto basePagePtr = getBasePage(id);
    if (!basePagePtr) {
        return;
    }
    if (basePagePtr->ownsButtons()) {
        setButtonLayout({ QWizard::Stretch });
    } else if (id == startId()) {
        // Nothing to go back to on the first page.
        setButtonLayout({ QWizard::Stretch, QWizard::NextButton, QWizard::FinishButton });
    } else if (basePagePtr->wantsRefreshButton()) {
        setButtonLayout({ QWizard::CustomButton1, QWizard::Stretch, QWizard::BackButton, QWizard::NextButton, QWizard::FinishButton });
        auto customButton = button(QWizard::CustomButton1);
        connect(customButton, &QAbstractButton::clicked, this, [this]() {
            auto basePagePtr = getCurrentBasePage();
            if (basePagePtr) {
                basePagePtr->refresh();
            }
        });
    } else {
        setButtonLayout({ QWizard::Stretch, QWizard::BackButton, QWizard::NextButton, QWizard::FinishButton });
    }
    // Classic style draws a rule above the buttons (and setButtonLayout shows it again), so it goes last:
    // KubikLauncher separates by space, not lines.
    // QWizardRuler is private and has no Q_OBJECT, so it is found by its C++ type name.
    for (auto* child : findChildren<QWidget*>()) {
        if (QByteArray(typeid(*child).name()).contains("QWizardRuler")) {
            child->hide();
        }
    }
}

void SetupWizard::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWizard::changeEvent(event);
}

SetupWizard::~SetupWizard() {}

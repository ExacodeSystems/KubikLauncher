#pragma once

#include <QCoreApplication>
#include <QEvent>
#include <QWizard>
#include <QWizardPage>

class BaseWizardPage : public QWizardPage {
   public:
    explicit BaseWizardPage(QWidget* parent = nullptr) : QWizardPage(parent) {}
    virtual ~BaseWizardPage() {};

    virtual bool wantsRefreshButton() { return false; }
    virtual void refresh() {}
    /** A page with its own buttons (the Microsoft sign-in page) hides the wizard's Back, Next and Finish. */
    virtual bool ownsButtons() { return false; }

   protected:
    virtual void retranslate() = 0;
    void changeEvent(QEvent* event) override
    {
        if (event->type() == QEvent::LanguageChange) {
            retranslate();
        }
        QWizardPage::changeEvent(event);
    }
    /** "Step 1 of 2", or nothing when the wizard has a single page. */
    QString stepText() const
    {
        if (!wizard() || wizard()->pageIds().size() < 2) {
            return {};
        }
        const int index = wizard()->pageIds().indexOf(wizard()->currentId());
        return QCoreApplication::translate("SetupWizard", "Step %1 of %2").arg(index < 0 ? 1 : index + 1).arg(wizard()->pageIds().size());
    }
};

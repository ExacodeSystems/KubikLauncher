#include "SetupWizard.h"

#include "JavaWizardPage.h"
#include "LanguageWizardPage.h"

#include <Application.h>
#include <FileSystem.h>
#include "translations/TranslationsModel.h"

#include <BuildConfig.h>
#include <QAbstractButton>
#include <QPainter>
#include <QResizeEvent>

#include <typeinfo>

#include "ui/kubik/KubikPaint.h"

namespace {

// KubikLauncher's first window is a small title screen: the lockup in a band of sky at the top, the
// pages under it, and the ground along the bottom edge, under the buttons.
constexpr int TITLE_BAND = 72;
constexpr int GROUND = 34;
constexpr int ICON = 30;
constexpr int WORDMARK = 24;

/** The pages KubikLauncher wrote for the sky. Prism's own (Java, paste, theme: repairs, rarely seen)
 *  have unstyled words that only read on the dark, so they keep it. */
bool onSky(const QWizardPage* page)
{
    return page && (page->objectName() == QLatin1String("languagePage") || page->objectName() == QLatin1String("loginPage"));
}

}  // namespace

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
    // The sky comes and goes with the page (onSky).
    update();
}

void SetupWizard::paintEvent(QPaintEvent* event)
{
    QWizard::paintEvent(event);
    using KubikPaint::Sprite;
    QPainter p(this);
    const qreal w = width();
    const qreal groundTop = height() - GROUND;
    if (onSky(currentPage())) {
        KubikPaint::drawDaySky(&p, QRectF(0, 0, w, groundTop));
        KubikPaint::drawSprite(&p, Sprite::Sun, QPointF(w - 24 - 40, 18), 5);
        KubikPaint::drawSprite(&p, Sprite::Cloud, QPointF(w - 190, 30), 4, 0.9);
        // On the horizon at the left: the buttons stand on the right.
        KubikPaint::drawSprite(&p, Sprite::Oak, QPointF(28, groundTop - 39 + 4), 3);
    }
    KubikPaint::drawGround(&p, QRectF(0, groundTop, w, GROUND), false);

    // The lockup, as on kubik.gg/launcher: the icon over its hard shadow, then the name.
    const QRectF icon(24, (TITLE_BAND - ICON) / 2.0 + 4, ICON, ICON);
    KubikPaint::drawLockupIcon(&p, APPLICATION->logo(), icon);
    const QSizeF name = KubikPaint::wordmarkSize(WORDMARK);
    KubikPaint::drawWordmark(&p, QPointF(icon.right() + 10, icon.center().y() - name.height() / 2 + 1), WORDMARK);
}

void SetupWizard::resizeEvent(QResizeEvent* event)
{
    QWizard::resizeEvent(event);
    // QWizard lays out its pages and buttons in one private child that it sizes to the whole window, and
    // ignores contentsMargins; it is fitted between the lockup and the ground here instead. Like
    // QWizardRuler below, it has no Q_OBJECT, so it is found by its C++ type name.
    for (auto* child : findChildren<QWidget*>(Qt::FindDirectChildrenOnly)) {
        if (QByteArray(typeid(*child).name()).contains("AntiFlicker")) {
            child->setGeometry(0, TITLE_BAND, width(), height() - TITLE_BAND - GROUND);
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

#include "Localization.h"
#include "Settings.h"
#include <windows.h>

namespace
{
    // Column order: English, Turkish, German, Ukrainian, Russian.
    constexpr const wchar_t* translations[][5] =
    {
        {L"General", L"Genel", L"Allgemein", L"Загальні", L"Общие"},
        {L"Toolbar", L"Araç çubuğu", L"Werkzeugleiste", L"Панель інструментів", L"Панель инструментов"},
        {L"About", L"Hakkımızda", L"Über uns", L"Про нас", L"О нас"},
        {L"PowerToolbar settings", L"PowerToolbar ayarları", L"PowerToolbar-Einstellungen", L"Налаштування PowerToolbar", L"Настройки PowerToolbar"},
        {L"Window controls, within reach.", L"Pencere kontrolü elinin altında.", L"Fenstersteuerung in Reichweite.", L"Керування вікнами завжди поруч.", L"Управление окнами всегда рядом."},
        {L"Display language", L"Arayüz dili", L"Anzeigesprache", L"Мова інтерфейсу", L"Язык интерфейса"},
        {L"Start when Windows starts", L"Windows açıldığında başlat", L"Mit Windows starten", L"Запускати разом із Windows", L"Запускать вместе с Windows"},
        {L"Start hidden in the system tray", L"Sistem tepsisinde gizli başlat", L"Im Infobereich ausgeblendet starten", L"Запускати приховано в системному треї", L"Запускать скрыто в системном трее"},
        {L"Make PowerToolbar feel at home.", L"PowerToolbar'ı kendine göre ayarla.", L"Richten Sie PowerToolbar nach Ihren Wünschen ein.", L"Налаштуйте PowerToolbar для себе.", L"Настройте PowerToolbar под себя."},
        {L"Your tools. Your order.", L"Senin araçların. Senin düzenin.", L"Ihre Werkzeuge. Ihre Reihenfolge.", L"Ваші інструменти. Ваш порядок.", L"Ваши инструменты. Ваш порядок."},
        {L"Select a tool to show, hide or move it.", L"Göstermek, gizlemek veya taşımak için bir araç seç.", L"Werkzeug auswählen, einblenden, ausblenden oder verschieben.", L"Виберіть інструмент, щоб показати, приховати або перемістити.", L"Выберите инструмент, чтобы показать, скрыть или переместить."},
        {L"Move up", L"Yukarı taşı", L"Nach oben", L"Вище", L"Выше"},
        {L"Move down", L"Aşağı taşı", L"Nach unten", L"Нижче", L"Ниже"},
        {L"Show / hide", L"Göster / gizle", L"Ein / aus", L"Показати / сховати", L"Показать / скрыть"},
        {L"Reset layout", L"Düzeni sıfırla", L"Zurücksetzen", L"Скинути порядок", L"Сбросить порядок"},
        {L"Save changes", L"Değişiklikleri kaydet", L"Änderungen speichern", L"Зберегти зміни", L"Сохранить изменения"},
        {L"Cancel", L"İptal", L"Abbrechen", L"Скасувати", L"Отмена"},
        {L"Settings saved.", L"Ayarlar kaydedildi.", L"Einstellungen gespeichert.", L"Налаштування збережено.", L"Настройки сохранены."},
        {L"Visit our website", L"Web sitemizi ziyaret et", L"Unsere Website besuchen", L"Відвідати наш сайт", L"Посетить наш сайт"},
        {L"Small tools. More focus.\n\nPowerToolbar puts everyday window controls just above your active window. Built by Axinomyus for a calmer, more focused Windows workspace.", L"Küçük araçlar. Daha fazla odak.\n\nPowerToolbar, günlük pencere kontrollerini etkin pencerenin hemen üstüne taşır. Daha sade ve odaklı bir Windows çalışma alanı için Axinomyus tarafından geliştirildi.", L"Kleine Werkzeuge. Mehr Fokus.\n\nPowerToolbar platziert die wichtigsten Fensterfunktionen direkt über dem aktiven Fenster. Von Axinomyus für einen übersichtlichen Windows-Arbeitsplatz entwickelt.", L"Невеликі інструменти. Більше зосередженості.\n\nPowerToolbar розміщує основні команди над активним вікном. Створено Axinomyus для зручнішого робочого простору Windows.", L"Небольшие инструменты. Больше внимания делу.\n\nPowerToolbar размещает основные команды над активным окном. Создано Axinomyus для удобного рабочего пространства Windows."},
        {L"Version 1.1.0", L"Sürüm 1.1.0", L"Version 1.1.0", L"Версія 1.1.0", L"Версия 1.1.0"},
        {L"MIT License", L"MIT Lisansı", L"MIT-Lizenz", L"Ліцензія MIT", L"Лицензия MIT"},
        {L"Runs locally. No account. No tracking.", L"Yerel çalışır. Hesap yok. Takip yok.", L"Lokal. Ohne Konto. Ohne Tracking.", L"Локально. Без облікового запису. Без відстеження.", L"Локально. Без аккаунта. Без отслеживания."},
        {L"Open PowerToolbar", L"PowerToolbar'ı aç", L"PowerToolbar öffnen", L"Відкрити PowerToolbar", L"Открыть PowerToolbar"},
        {L"Compact toolbar", L"Kompakt çubuk", L"Kompakte Leiste", L"Компактна панель", L"Компактная панель"},
        {L"Hide to tray", L"Tepsiye gizle", L"In den Infobereich", L"Сховати в трей", L"Скрыть в трей"},
        {L"Exit", L"Çıkış", L"Beenden", L"Вийти", L"Выход"},
        {L"Leave window fullscreen", L"Pencerenin tam ekranını kapat", L"Vollbild beenden", L"Вийти з повноекранного режиму", L"Выйти из полноэкранного режима"},
        {L"Expand PowerToolbar", L"PowerToolbar'ı genişlet", L"PowerToolbar erweitern", L"Розгорнути PowerToolbar", L"Развернуть PowerToolbar"},
        {L"Keep on top", L"Üstte tut", L"Im Vordergrund", L"Поверх інших", L"Поверх остальных"},
        {L"Center window", L"Pencereyi ortala", L"Fenster zentrieren", L"Центрувати вікно", L"Центрировать окно"},
        {L"Fullscreen", L"Tam ekran", L"Vollbild", L"Повний екран", L"Полный экран"},
        {L"Minimize window", L"Pencereyi küçült", L"Fenster minimieren", L"Згорнути вікно", L"Свернуть окно"},
        {L"Close window", L"Pencereyi kapat", L"Fenster schließen", L"Закрити вікно", L"Закрыть окно"},
        {L"End app", L"Uygulamayı sonlandır", L"App beenden", L"Завершити програму", L"Завершить приложение"},
        {L"Mute / unmute app", L"Uygulama sesini aç / kapat", L"App-Ton ein / aus", L"Звук програми: увімк. / вимк.", L"Звук приложения: вкл. / выкл."},
        {L"Keep this window on top", L"Bu pencereyi diğerlerinin üstünde tut", L"Dieses Fenster im Vordergrund halten", L"Тримати це вікно поверх інших", L"Держать это окно поверх остальных"},
        {L"Center on this monitor", L"Bu monitörde ortala", L"Auf diesem Monitor zentrieren", L"Центрувати на цьому моніторі", L"Центрировать на этом мониторе"},
        {L"Toggle this window's fullscreen mode", L"Bu pencerenin tam ekranını aç veya kapat", L"Vollbild für dieses Fenster ein- oder ausschalten", L"Увімкнути або вимкнути повний екран для цього вікна", L"Включить или выключить полный экран для этого окна"},
        {L"Minimize window", L"Pencereyi küçült", L"Fenster minimieren", L"Згорнути вікно", L"Свернуть окно"},
        {L"Close window", L"Pencereyi kapat", L"Fenster schließen", L"Закрити вікно", L"Закрыть окно"},
        {L"End all processes for this app...", L"Bu uygulamanın tüm işlemlerini sonlandır...", L"Alle Prozesse dieser App beenden...", L"Завершити всі процеси цієї програми...", L"Завершить все процессы этого приложения..."},
        {L"Switch to compact toolbar", L"Kompakt çubuğa geç", L"Zur kompakten Leiste wechseln", L"Перейти до компактної панелі", L"Перейти к компактной панели"},
        {L"Hide until you open PowerToolbar from the tray", L"Tepsiden PowerToolbar'ı açana kadar gizle", L"Bis zum Öffnen über den Infobereich ausblenden", L"Сховати до відкриття PowerToolbar із трея", L"Скрыть до открытия PowerToolbar из трея"},
        {L"Mute or unmute this app's audio sessions", L"Bu uygulamanın sesini aç veya kapat", L"Audio dieser App stummschalten oder aktivieren", L"Увімкнути або вимкнути звук цієї програми", L"Включить или выключить звук этого приложения"},
        {L"Visible", L"Görünür", L"Sichtbar", L"Видимо", L"Видимо"},
        {L"Hidden", L"Gizli", L"Ausgeblendet", L"Приховано", L"Скрыто"},
        {L"App audio is optional and hidden by default. It affects this app's audio sessions, not your system volume.", L"Ses düğmesi isteğe bağlıdır ve başlangıçta gizlidir. Sistem sesini değil, seçili uygulamanın ses oturumlarını etkiler.", L"App-Audio ist optional und anfangs ausgeblendet. Es steuert die Audiositzungen der App, nicht die Systemlautstärke.", L"Кнопка звуку необов'язкова й спочатку прихована. Вона керує звуком програми, а не гучністю системи.", L"Кнопка звука необязательна и изначально скрыта. Она управляет звуком приложения, а не громкостью системы."},
        {L"The toolbar normally stays outside windows. In fullscreen or maximized windows, move to the top center and click the arrow to open it temporarily. Move away to hide it.", L"Çubuk normalde pencerenin dışında kalır. Tam ekran veya büyütülmüş pencerede üst ortaya gelip oka tıklayarak geçici açabilirsin. Fareyi uzaklaştırınca gizlenir.", L"Die Leiste bleibt normalerweise außerhalb des Fensters. Bei Vollbild oder maximierten Fenstern oben mittig den Pfeil anklicken. Beim Wegbewegen wird sie ausgeblendet.", L"Панель зазвичай поза вікном. У повноекранному чи розгорнутому вікні наведіть курсор угору по центру й натисніть стрілку. Відведіть курсор, щоб сховати панель.", L"Панель обычно вне окна. В полноэкранном или развёрнутом окне наведите курсор вверх по центру и нажмите стрелку. Отведите курсор, чтобы скрыть панель."},
        {L"Could not save settings. Your previous settings have been kept.", L"Ayarlar kaydedilemedi. Önceki ayarların korundu.", L"Einstellungen konnten nicht gespeichert werden. Die bisherigen bleiben erhalten.", L"Не вдалося зберегти налаштування. Попередні налаштування збережено.", L"Не удалось сохранить настройки. Прежние настройки сохранены."},
        {L"Could not complete this action.", L"Bu işlem tamamlanamadı.", L"Diese Aktion konnte nicht abgeschlossen werden.", L"Не вдалося виконати дію.", L"Не удалось выполнить действие."},
        {L"The window may have closed or may require elevated permissions.", L"Pencere kapanmış olabilir veya yönetici izni gerekebilir.", L"Das Fenster wurde möglicherweise geschlossen oder benötigt erhöhte Rechte.", L"Вікно могло закритися або потребувати прав адміністратора.", L"Окно могло закрыться или требовать прав администратора."},
        {L"End every running process for this app? Unsaved work will be lost.", L"Bu uygulamanın tüm işlemleri sonlandırılsın mı? Kaydedilmemiş çalışmalar kaybolacak.", L"Alle Prozesse dieser App beenden? Nicht gespeicherte Arbeit geht verloren.", L"Завершити всі процеси цієї програми? Незбережені дані буде втрачено.", L"Завершить все процессы этого приложения? Несохранённые данные будут потеряны."},
        {L"Windows Explorer manages your desktop. Use Close window to close individual Explorer windows.", L"Windows Gezgini masaüstünü yönetir. Tek bir Gezgini kapatmak için Pencereyi kapat düğmesini kullan.", L"Windows-Explorer verwaltet den Desktop. Einzelne Fenster über Fenster schließen beenden.", L"Провідник Windows керує робочим столом. Закривайте окремі вікна командою Закрити вікно.", L"Проводник Windows управляет рабочим столом. Закрывайте отдельные окна командой Закрыть окно."},
        {L"No audio session was found for this app. Start playing audio, then try again.", L"Bu uygulama için ses oturumu bulunamadı. Ses oynatıp tekrar dene.", L"Keine Audiositzung gefunden. Starten Sie die Wiedergabe und versuchen Sie es erneut.", L"Аудіосесію не знайдено. Почніть відтворення звуку та спробуйте ще раз.", L"Аудиосессия не найдена. Начните воспроизведение звука и повторите попытку."},
        {L"Could not change this app's audio. Some audio sessions may be unavailable; check Windows Volume Mixer.", L"Uygulama sesi değiştirilemedi. Bazı ses oturumları erişilemez olabilir; Windows Ses Karıştırıcısı'nı kontrol et.", L"App-Audio konnte nicht geändert werden. Prüfen Sie die Audiositzungen im Windows-Lautstärkemixer.", L"Не вдалося змінити звук програми. Перевірте аудіосесії в мікшері гучності Windows.", L"Не удалось изменить звук приложения. Проверьте аудиосессии в микшере громкости Windows."},
        {L"Windows could not add the tray icon. Please try again.", L"Windows tepsi simgesini ekleyemedi. Lütfen tekrar dene.", L"Das Symbol konnte nicht zum Infobereich hinzugefügt werden.", L"Не вдалося додати значок у трей. Спробуйте ще раз.", L"Не удалось добавить значок в трей. Повторите попытку."},
        {L"Could not open the website. Visit axinomyus.com/products/powertoolbar in your browser.", L"Site açılamadı. Tarayıcından axinomyus.com/products/powertoolbar adresine git.", L"Website nicht verfügbar. Öffnen Sie axinomyus.com/products/powertoolbar im Browser.", L"Не вдалося відкрити сайт. Перейдіть на axinomyus.com/products/powertoolbar у браузері.", L"Не удалось открыть сайт. Перейдите на axinomyus.com/products/powertoolbar в браузере."},
        {L"Windows startup is managed for all users. Change it by running PowerToolbar as administrator.", L"Başlangıç ayarı tüm kullanıcılar için yönetiliyor. Değiştirmek için PowerToolbar'ı yönetici olarak çalıştır.", L"Autostart wird für alle Benutzer verwaltet. Starten Sie PowerToolbar als Administrator, um ihn zu ändern.", L"Автозапуск налаштовано для всіх користувачів. Для зміни запустіть PowerToolbar від адміністратора.", L"Автозапуск настроен для всех пользователей. Для изменения запустите PowerToolbar от администратора."},
        {L"PowerToolbar could not start. Please try again.", L"PowerToolbar başlatılamadı. Lütfen tekrar dene.", L"PowerToolbar konnte nicht starten. Bitte erneut versuchen.", L"Не вдалося запустити PowerToolbar. Спробуйте ще раз.", L"Не удалось запустить PowerToolbar. Повторите попытку."},
        {L"Capture window content", L"Pencere içi ekran görüntüsü", L"Fensterinhalt aufnehmen", L"Знімок вмісту вікна", L"Снимок содержимого окна"},
        {L"Save visible window content as PNG, without its title bar", L"Görünür pencere içeriğini başlık çubuğu olmadan PNG olarak kaydet", L"Sichtbaren Fensterinhalt ohne Titelleiste als PNG speichern", L"Зберегти видимий вміст вікна без заголовка як PNG", L"Сохранить видимое содержимое окна без заголовка как PNG"},
        {L"Could not save the screenshot. Keep the entire window on screen and choose a writable folder. Protected content cannot be captured.", L"Ekran görüntüsü kaydedilemedi. Pencerenin tamamını ekranda tut ve yazılabilir bir klasör seç. Korumalı içerik yakalanamaz.", L"Screenshot konnte nicht gespeichert werden. Das Fenster muss vollständig sichtbar sein. Wählen Sie einen beschreibbaren Ordner. Geschützte Inhalte sind ausgeschlossen.", L"Не вдалося зберегти знімок. Вікно має бути повністю на екрані. Виберіть доступну для запису папку. Захищений вміст не знімається.", L"Не удалось сохранить снимок. Окно должно быть целиком на экране. Выберите доступную для записи папку. Защищённое содержимое не снимается."},
        {L"Mute this app", L"Bu uygulamanın sesini kapat", L"Diese App stummschalten", L"Вимкнути звук цієї програми", L"Выключить звук этого приложения"},
        {L"Unmute this app", L"Bu uygulamanın sesini aç", L"Ton dieser App einschalten", L"Увімкнути звук цієї програми", L"Включить звук этого приложения"},
        {L"Window capture is optional and hidden by default. Save visible content as PNG, without the title bar or toolbar.", L"Ekran görüntüsü düğmesi isteğe bağlıdır ve başlangıçta gizlidir. Görünür içeriği başlık ve araç çubuğu olmadan PNG olarak kaydeder.", L"Fensteraufnahme ist optional und anfangs ausgeblendet. Sichtbaren Inhalt ohne Titel- und Werkzeugleiste als PNG speichern.", L"Знімок вікна необов'язковий і спочатку прихований. Зберігає видимий вміст як PNG без заголовка й панелі інструментів.", L"Снимок окна необязателен и изначально скрыт. Сохраняет видимое содержимое как PNG без заголовка и панели инструментов."},
        {L"Open PowerToolbar inside this window", L"PowerToolbar'ı bu pencerenin içinde aç", L"PowerToolbar in diesem Fenster öffnen", L"Відкрити PowerToolbar у цьому вікні", L"Открыть PowerToolbar внутри этого окна"},
        {L"Collapse PowerToolbar", L"PowerToolbar'ı geri gizle", L"PowerToolbar einklappen", L"Згорнути PowerToolbar", L"Свернуть PowerToolbar"}
    };

    static_assert(sizeof(translations) / sizeof(translations[0]) == static_cast<size_t>(Text::Count));
}

const wchar_t* Translate(Text text, Language language)
{
    size_t column = static_cast<size_t>(language);
    if (column >= static_cast<size_t>(Language::Count))
    {
        column = 0;
    }
    const size_t row = static_cast<size_t>(text);
    if (row >= static_cast<size_t>(Text::Count))
    {
        return L"PowerToolbar";
    }
    return translations[row][column];
}

const wchar_t* Tr(Text text)
{
    return Translate(text, GetSettings().language);
}

const wchar_t* LanguageName(Language language)
{
    constexpr const wchar_t* names[] = { L"English", L"Türkçe", L"Deutsch", L"Українська", L"Русский" };
    if (language < Language::English || language >= Language::Count)
    {
        return names[0];
    }
    return names[static_cast<size_t>(language)];
}

Language DetectLanguage()
{
    switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
    {
    case LANG_TURKISH:
        return Language::Turkish;
    case LANG_GERMAN:
        return Language::German;
    case LANG_UKRAINIAN:
        return Language::Ukrainian;
    case LANG_RUSSIAN:
        return Language::Russian;
    }
    return Language::English;
}

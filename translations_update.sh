echo "Updating translation files..."

# Обновление .ts файлов
echo "Updating .ts files..."
lupdate . -ts translations/app_en.ts translations/app_ru.ts

# Компиляция .ts в .qm
echo "Compiling .qm files..."
lrelease translations/app_en.ts -qm translations/app_en.qm
lrelease translations/app_ru.ts -qm translations/app_ru.qm
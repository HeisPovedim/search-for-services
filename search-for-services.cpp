#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <locale>
#include <codecvt>

using namespace std;

struct Parameters {
    int startMonth;         // начало месяца
    int endMonth;           // конец месяца
    wstring customersFile;  // customers.txt
    wstring servicesFile;   // services.txt
    wstring usageFile;      // information_services.txt
    wstring reportFile;     // report.txt
    bool showDebt;          // показать клиенту долг
    bool showCredit;        // показать клиенту кредит
    wstring dateFormat;     // формат даты
}; // инициализация параметров из Param.ini

// NOTE: Функция чтение параметров 
Parameters readParameters() {
    Parameters params;
    wifstream paramFile("Param.ini");
    paramFile.imbue(locale(locale(), new codecvt_utf8<wchar_t>));

    wstring line;
    while (getline(paramFile, line)) {
        if (line[0] == L'#' || line.empty()) continue;

        size_t pos = line.find(L'=');
        if (pos != wstring::npos) {
            wstring key = line.substr(0, pos);
            wstring value = line.substr(pos + 1);

            // присвоение значений
            if (key == L"START_QUARTER_MONTH") params.startMonth = stoi(value);
            if (key == L"END_QUARTER_MONTH") params.endMonth = stoi(value);
            if (key == L"CUSTOMERS_FILE") params.customersFile = value;
            if (key == L"SERVICES_FILE") params.servicesFile = value;
            if (key == L"USAGE_FILE") params.usageFile = value;
            if (key == L"REPORT_FILE") params.reportFile = value;
            if (key == L"SHOW_CUSTOMER_DEBT") params.showDebt = (value == L"true");
            if (key == L"SHOW_CUSTOMER_CREDIT") params.showCredit = (value == L"true");
            if (key == L"DATE_FORMAT") params.dateFormat = value;
        }
    }
    return params;
}

struct customer {
    wstring fio;
    wstring phone;
    wstring startDate;
    wstring endDate;
    double debt;
    double credit;
}; // инициализация данных пользователя из customers.txt

struct service {
    wstring name;
    wstring code;
    double tarif;
    wstring timeInterval; // "мин.", "сутки", "месяц" или "#"
}; // инициализация данных об услугах из services.txt

struct information_service {
    wstring phone;
    wstring serviceCode;
    wstring dateTime; // полная дата и время
    wstring usageTime;
}; // инициализация данных об услугах из information_services.txt

// NOTE: Функция проверки нахождения пользователя во втором квартале
bool isInSecondQuarter(const wstring& date, const Parameters& params) {

    // проверка формата даты yyyy-mm-dd
    if (date.length() != 10 || date[4] != L'-' || date[7] != L'-') {
        return false;
    }

    // извлечение месяца из даты
    int month = stoi(date.substr(5, 2));
    int year = stoi(date.substr(0, 4));

    // получаем значения начала и конца квартала из параметров
    int startMonth = params.startMonth;
    int endMonth = params.endMonth;

    // проверяем, что месяц попадает в заданный диапазон
    return (month >= startMonth && month <= endMonth);
}

wstring convertDateFormat(const wstring& date, const wstring& dateFormat) {
    if (date.length() != 10 || date[4] != L'-' || date[7] != L'-') {
        return L"Invalid date"; // проверка на корректный формат
    }

    wstring year = date.substr(0, 4);
    wstring month = date.substr(5, 2);
    wstring day = date.substr(8, 2);

    // изменение формата даты в зависимости от параметра dateFormat
    if (dateFormat == L"DD-MM-YYYY") {
        return day + L"-" + month + L"-" + year;
    }
    else if (dateFormat == L"MM-DD-YYYY") {
        return month + L"-" + day + L"-" + year;
    }
    else {
        // параметр по умолчанию, если указанный формат не распознается
        return year + L"-" + month + L"-" + day;
    }
}

int main() {
    setlocale(LC_ALL, "Russian");

    Parameters params = readParameters();

    // Установка кодировки для работы с UTF-8
    wifstream clientsFile(params.customersFile);
    clientsFile.imbue(locale(locale(), new codecvt_utf8<wchar_t>));

    wifstream servicesFile(params.servicesFile);
    servicesFile.imbue(locale(locale(), new codecvt_utf8<wchar_t>));

    wifstream usageFile(params.usageFile);
    usageFile.imbue(locale(locale(), new codecvt_utf8<wchar_t>));

    wofstream reportFile(params.reportFile);
    reportFile.imbue(locale(locale(), new codecvt_utf8<wchar_t>));


    vector<customer> clients;
    vector<service> services;
    vector<information_service> usages;

    // NOTE: Чтение клиентов
    wstring line;
    while (getline(clientsFile, line)) {
        wstringstream ss(line);
        customer client;
        getline(ss, client.fio, L',');
        getline(ss, client.phone, L',');
        getline(ss, client.startDate, L',');
        getline(ss, client.endDate, L',');
        ss >> client.debt; ss.ignore();
        ss >> client.credit;

        if (isInSecondQuarter(client.startDate, params)) {
            clients.push_back(client);
        }
    }

    // NOTE: Чтение услуг
    while (getline(servicesFile, line)) {
        wstringstream ss(line);
        service service;
        getline(ss, service.name, L',');
        getline(ss, service.code, L',');
        ss >> service.tarif; ss.ignore();
        getline(ss, service.timeInterval);
        services.push_back(service);
    }

    // NOTE: Чтение использования услуг
    while (getline(usageFile, line)) {
        wstringstream ss(line);
        information_service usage;
        getline(ss, usage.phone, L',');
        getline(ss, usage.serviceCode, L',');
        getline(ss, usage.dateTime);
        getline(ss, usage.usageTime);
        usages.push_back(usage);
    }

    reportFile << "\xEF\xBB\xBF"; // запись BOM для UTF-8
    reportFile << L"Customer service report:\n";

    for (const auto& client : clients) {
        reportFile << client.fio << L", " << client.phone;

        if (params.showDebt) {
            reportFile << L", Долг: " << client.debt;
        }
        if (params.showCredit) {
            reportFile << L", Кредит: " << client.credit;
        }
        reportFile << L"\n";

        bool hasServices = false; // флаг услуг

        for (const auto& usage : usages) {
            if (usage.phone == client.phone) {
                hasServices = true; // есть услуги

                for (const auto& service : services) {
                    if (usage.serviceCode == service.code) {
                        // извлечение даты из полного формата даты и времени
                        wstring datePart = usage.dateTime.substr(0, 10); // Получаем YYYY-MM-DD
                        wstring dateFormat = params.dateFormat;
                        wstring formattedDate = convertDateFormat(datePart, params.dateFormat);

                        if (formattedDate != L"Invalid date") {
                            reportFile << L"- " << service.name << L", " << service.tarif
                                << L", " << formattedDate
                                << L", " << usage.usageTime << L"\n";
                        }
                        else {
                            reportFile << L"- " << service.name << L", " << service.tarif
                                << L", Неверная дата"
                                << L", " << usage.usageTime << L"\n";
                        }
                    }
                }
            }
        }

        if (!hasServices) {
            reportFile << L"- Услуги отсутствуют\n";
        }

        reportFile << L"\n"; // пустая строка между клиентами
    }

    reportFile.close(); // проверка наличия данных

    // NOTE: Открываем файл для чтения и проверяем его содержимое
    wifstream checkReport("report.txt");
    checkReport.imbue(locale(locale(), new codecvt_utf8<wchar_t>));

    if (checkReport.peek() == wifstream::traits_type::eof()) {
        cout << "Отчет пуст." << endl;
    }
    else {
        cout << "Отчет успешно создан." << endl;
    }

    return 0;
}
// Дадим пинам соответствующие имена, для того, чтобы было легче читать программу
#pragma region pin_names
#define BUZZER A5 //зуммер
#define TERM A3 // термометр

// дисплей
#define DIO 10
#define CLK 12

// светодиоды
#define YELLOW_LED A2 // жёлтый светодиод
#define RED_LED A1 // красный светодиод
#define BLUE_LED A0 // синий светодиод

// кнопки
#define BTN_PLUS 4// кнопка "+"
#define BTN_MINUS 3 // кнопка "-"
#define BTN_SET 2 // кнопка "set"
#define BTN_RESET 5 // кнопка "reset"

// реле
#define TEN_RELAY 11 // реле на ТЭН
#define PUMP_RELAY 9 // реле на помпу
#pragma endregion pin_names

// подключение библиотеки таймеров
#include <TimerMs.h>

// настройка дисплея
#pragma region display_settings
#include "GyverTM1637.h"
GyverTM1637 disp(CLK, DIO);

uint32_t Now, clocktimer;
bool flag;

bool point_on = false;

// таймер для мигания точек
#define POINT_BLINK_TIME 200
TimerMs point_blink_tmr(POINT_BLINK_TIME, true);

#pragma endregion display_settings

// настройка термометра
#pragma region thermometer_settings
// подключение библиотеки для термометра
#include <microDS18B20.h>
MicroDS18B20<TERM> sensor;

// допустимая погрешность для градуса
#define TEMP_DELTA 0

#define TEMP_MAX_DELTA 5
#define TEMP_MIN_DELTA 5

// текущая температура градусника
int temperature;

// время на замер температуры
#define TEMP_TIME 250
TimerMs temp_timer(TEMP_TIME, true);

// ограничения задаваемой температуры (минимум и максимум)
#define TEMP_MIN 0
#define TEMP_MAX 100

// ограничения задаваемого времени периода в минутах
#define TIME_MIN 0
#define TIME_MAX 9999

#pragma endregion thermometer_settings

// длина писка для зуммера
#define BUZZER_BEEP_TIME 500
TimerMs buzzer_timer(BUZZER_BEEP_TIME, true);
int buzzer_cnt = -1;
bool buzzer_on = false;

// флаги кнопок
bool btn_plus_pressed = false;
bool btn_minus_pressed = false;
bool btn_set_pressed = false;
bool btn_reset_pressed = false;

// длина блокировки кнопок после нажатия
#define DEFAULT_BTN_LOCK_TIME 200
TimerMs btn_lock_timer(0, 0, 1);

#pragma region menu_settings
// индексы технических сообщеий и имена по элементам
enum alert_items:int {
  no_program,
  no_empty_slots
};

byte alert_names[2][4] = {
  {0x76, 0x37, 0x73, 0x0},
  {0x37, 0x73, 0x4F, 0x37}
};

// флаг отображения техническое сообщение
bool display_alert;

// индекс отображаемого технического сообщения
int alert_index;

// таймер на отображение технического сообщения
uint32_t alert_timer;

// индексы меню и имена элементов по индексам
enum menu_items:int {
  m_settings, // нас (настройки)
  m_work, // раб (работа)
  s_program, // прог (вывести программу)
  s_reset, // рес (сбросить программу)
  s_redact, // ре (редактировать программу)
  s_add, // нов (добавить новый элемент в программу)
  s_remove, // удл (удалить элемент из программы)
  w_pause, // пауз (остановить работу программы)
  w_resume, // прод (продолжить/начать работу программы)
  w_show, // по (показать текущую программу)
  w_current, // пс (показать текущий шаг программы + текущую температуру)
  w_reset_prog, // рес (вернуть программу на начало)
  w_prod_pump // прн (продуть насос для того, чтобы убрать водяную пробку)
};

byte menu_names[13][4] = {
  {0x76, 0x77, 0x39, 0x0},
  {0x73, 0x77, 0x7D, 0x0},
  {0x37, 0x73, 0x3F, 0x31},
  {0x73, 0x79, 0x39, 0x0},
  {0x73, 0x79, 0x0, 0x0},
  {0x76, 0x3F, 0x7F, 0x0},
  {0x72, 0x7D, 0x73, 0x0},
  {0x37, 0x77, 0x72, 0x4F},
  {0x37, 0x73, 0x3F, 0x0},
  {0x37, 0x3F, 0x0, 0x0},
  {0x37, 0x39, 0x0, 0x0},
  {0x73, 0x79, 0x39, 0x0},
  {0x37, 0x73, 0x76, 0x0}
};

// индексы меню
enum menus:int {
  main,
  settings,
  work,
  program_index,
  program_temp,
  program_time,
  program_redact_index,
  program_redact_temp,
  program_redact_time,
  program_add_index,
  program_add_temp,
  program_add_time,
  program_remove_index,
  show_index,
  show_temp,
  show_time,
  show_current_index,
  show_current_temp,
  show_current_desired_temp,
  show_current_time
};

// выбранное на данный момент меню
int current_menu;

// массивы элементов для каждого меню
#define MAIN_MENU_LEN 2
int current_main_menu_index = 0;
int main_menu_elems[MAIN_MENU_LEN] = {
  m_settings,
  m_work
};

#define SETTINGS_MENU_LEN 5
int current_settings_menu_index = 0;
int settings_menu_elems[SETTINGS_MENU_LEN] = {
  s_program,
  s_reset,
  s_redact,
  s_add,
  s_remove
};

#define WORK_MENU_LEN 6
int current_work_menu_index = 0;
int work_menu_elems[WORK_MENU_LEN] = {
  w_resume,
  w_pause,
  w_show,
  w_current,
  w_prod_pump,
  w_reset_prog
};

// индекс текущего элемента меню
int current_menu_element_index;

#pragma endregion menu_settings

#pragma region program_settings

// режим реле на ТЭН (включён, выключен)
bool TEN_relay_on = false;

// режим реле на насос (включён, выключен)
bool pump_relay_on = false;

// таймер и время для прочистки насоса
#define PUMP_CLEAR_WORK_TIME 1000
#define PUMP_CLEAR_REST_TIME 1000
#define CLEAR_REPEATS_CNT 10

// флаг доведения температуры до нужной до включения таймера шага
bool correcting_temperature = false;

// флаг режима паузы
bool working;

// флаг достижения температуры шага
bool reached_step_temp = false;

// шаг программы
struct step {
  int temp; // температура на шаге
  int time; // длительность шага
};

// массив шагов
#define MAX_PROGRAM_LEN 20
int current_step_index = 0;
int current_step_time = 0;

int current_program_len = 0;
step program[MAX_PROGRAM_LEN];

// временная переменная для записи параметров нового шага
step new_step {0, 0};

// индекс шага программы для просмотра/редактирования
int current_step_show_index = 0;

// таймер шага
TimerMs step_timer(0, 0, 1);

#pragma endregion program_settings

// остановить программу
void stop_work(bool stop = true) {
  active_temperature_controll(false, false);
  if (reached_step_temp)
    step_timer.stop();
  working = false;
}

// продолжить работу
void resume_work() {
  working = true;
  if (reached_step_temp)
    step_timer.resume();
}

// проверка на то, существует ли программа
bool check_program_exists() {
  if (current_program_len == 0) {
    display_alert = true;
    alert_index = no_program;
    alert_timer = millis();
    return false;
  }
  return true;
}

// проверка на то, есть ли в программе свободные ячейки
bool check_program_empty_slots() {
  if (current_program_len == MAX_PROGRAM_LEN) {
    display_alert = true;
    alert_index = no_empty_slots;
    alert_timer = millis();
    return false;
  }
  return true;
}

#pragma region button_processing
// функция изменения флага кнопки по нажатию
void check_button(bool* flag, int input) {
  // если нажата
  if (!(*flag) && input)
    *flag = true;
  // если не нажата
  if (!input)
    *flag = false;
}

// обработка нажатия кнопки +
void process_plus() {
  // обработка перемещения по меню
  if (current_menu == main) {
    current_main_menu_index += 1;
    if (current_main_menu_index >= MAIN_MENU_LEN)
      current_main_menu_index = 0;
    current_menu_element_index = main_menu_elems[current_main_menu_index];
  }
  // обработка перемещения по меню настроек
  else if (current_menu == settings) {
    current_settings_menu_index += 1;
    if (current_settings_menu_index >= SETTINGS_MENU_LEN)
      current_settings_menu_index = 0;
    current_menu_element_index = settings_menu_elems[current_settings_menu_index];
  }
  // движение по программе "направо" во время просмотра в режиме настройки
  else if (current_menu == program_index) {
    current_menu = program_temp;
  }
  else if (current_menu == program_temp) {
    current_menu = program_time;
  }
  else if (current_menu == program_time) {
    current_step_show_index += 1;
    if (current_step_show_index >= current_program_len)
      current_step_show_index = 0;
    current_menu = program_index;
  }
  // редактирование программы
  else if (current_menu == program_redact_index) {
    if (current_step_show_index < current_program_len)
      current_step_show_index += 1;
    if (current_step_show_index >= current_program_len)
      current_step_show_index = 0;
  }
  else if (current_menu == program_redact_temp) {
    if (program[current_step_show_index].temp <= TEMP_MAX)
      program[current_step_show_index].temp += 1;
    if (program[current_step_show_index].temp > TEMP_MAX)
      program[current_step_show_index].temp = TEMP_MIN;

  }
  else if (current_menu == program_redact_time) {
    if (program[current_step_show_index].time <= TIME_MAX)
      program[current_step_show_index].time += 1;
    if (program[current_step_show_index].time > TIME_MAX)
      program[current_step_show_index].time = TIME_MIN;
  }
  // добавление нового элемента
  else if (current_menu == program_add_index) {
    if (current_step_show_index <= current_program_len)
      current_step_show_index += 1;
    if (current_step_show_index > current_program_len)
      current_step_show_index = 0;
  }
  else if (current_menu == program_add_temp) {
    if (new_step.temp <= TEMP_MAX)
      new_step.temp += 1;
    if (new_step.temp > TEMP_MAX)
      new_step.temp = TEMP_MIN;
  }
  else if (current_menu == program_add_time) {
    if (new_step.time <= TIME_MAX)
      new_step.time += 1;
    if (new_step.time > TIME_MAX)
      new_step.time = TIME_MIN;
  }
  // удаление элемента
  else if (current_menu == program_remove_index) {
    if (current_step_show_index < current_program_len)
      current_step_show_index += 1;
    if (current_step_show_index >= current_program_len)
      current_step_show_index = 0;
  }
  // обработка перемещения по меню работы
  else if (current_menu == work) {
    current_work_menu_index += 1;
    if (current_work_menu_index >= WORK_MENU_LEN)
      current_work_menu_index = 0;
    current_menu_element_index = work_menu_elems[current_work_menu_index];
  }
  // перемещение по меню просмотра программы вправо
  else if (current_menu == show_index) {
    current_menu = show_temp;
  }
  else if (current_menu == show_temp) {
    current_menu = show_time;
  }
  else if (current_menu == show_time) {
    current_step_show_index += 1;
    if (current_step_show_index > current_program_len - 1)
      current_step_show_index = 0;
    current_menu = show_index;
  }
  // перемещение по меню просмотра текущего шага вправо
  else if (current_menu == show_current_index) {
    current_menu = show_current_desired_temp;
  }
  else if (current_menu == show_current_desired_temp) {
    current_menu = show_current_temp;
  }
  else if (current_menu == show_current_temp) {
    current_menu = show_current_time;
  }
  else if (current_menu == show_current_time) {
    current_menu = show_current_index;
  }
}

// обработка нажатия кнопки -
void process_minus() {
  // обработка перемещения по главному меню
  if (current_menu == main) {
    current_main_menu_index -= 1;
    if (current_main_menu_index < 0)
      current_main_menu_index = MAIN_MENU_LEN - 1;
    current_menu_element_index = main_menu_elems[current_main_menu_index];
  }
  // обработка перемещения по меню настроек
  else if (current_menu == settings) {
    current_settings_menu_index -= 1;
    if (current_settings_menu_index < 0)
      current_settings_menu_index = SETTINGS_MENU_LEN - 1;
    current_menu_element_index = settings_menu_elems[current_settings_menu_index];
  }
  // движение по программе "налево" во время просмотра в режиме настройки
  else if (current_menu == program_index) {
    current_step_show_index -= 1;
    if (current_step_index < 0)
      current_step_show_index = current_program_len - 1;
    current_menu = program_temp;
  }
  else if (current_menu == program_temp) {
    current_menu = program_time;
  }
  else if (current_menu == program_time) {
    current_menu = program_index;
  }
  // редактирование программы
  else if (current_menu == program_redact_index) {
    if (current_step_show_index >= 0 && current_program_len > 0)
      current_step_show_index -= 1;
    if (current_step_show_index < 0)
      current_step_show_index = current_program_len - 1;
  }
  else if (current_menu == program_redact_temp) {
    if (program[current_step_show_index].temp >= TEMP_MIN)
      program[current_step_show_index].temp -= 1;
    if (program[current_step_show_index].temp < TEMP_MIN)
      program[current_step_show_index].temp = TEMP_MAX;
  }
  else if (current_menu == program_redact_time) {
    if (program[current_step_show_index].time >= TIME_MIN)
      program[current_step_show_index].time -= 1;
    if (program[current_step_show_index].time < TIME_MIN)
      program[current_step_show_index].time = TIME_MAX;
  }
  // добавление нового элемента
  else if (current_menu == program_add_index) {
    if (current_step_show_index >= 0 && current_program_len > 0)
      current_step_show_index -= 1;
    if (current_step_show_index < 0)
      current_step_show_index = current_program_len;
  }
  else if (current_menu == program_add_temp) {
    if (new_step.temp >= TEMP_MIN)
      new_step.temp -= 1;
    if (new_step.temp < TEMP_MIN)
      new_step.temp = TEMP_MAX;
  }
  else if (current_menu == program_add_time) {
    if (new_step.time >= TIME_MIN)
      new_step.time -= 1;
    if (new_step.time < TIME_MIN)
      new_step.time = TIME_MAX;
  }
  // удаление элемента
  else if (current_menu == program_remove_index) {
    if (current_step_show_index >= 0 && current_program_len > 0)
      current_step_show_index -= 1;
    if (current_step_show_index < 0)
      current_step_show_index = current_program_len - 1;
  }
  // обработка перемещения по меню работы
  else if (current_menu == work) {
    current_work_menu_index -= 1;
    if (current_work_menu_index < 0)
      current_work_menu_index = WORK_MENU_LEN - 1;
    current_menu_element_index = work_menu_elems[current_work_menu_index];
  }
  // перемещение по меню просмотра программы влево
  else if (current_menu == show_index) {
    current_step_show_index -= 1;
    if (current_step_index < 0 && current_program_len > 0)
      current_step_show_index = current_program_len - 1;
    current_menu = show_time;
  }
  else if (current_menu == show_temp) {
    current_menu = show_index;
  }
  else if (current_menu == show_time) {
    current_menu = show_temp;    
  }
  // перемещение по меню просмотра текущего шага влево
  else if (current_menu == show_current_index) {
    current_menu = show_current_temp;
  }
  else if (current_menu == show_current_desired_temp) {
    current_menu = show_current_index;
  }
  else if (current_menu == show_current_temp) {
    current_menu = show_current_desired_temp;
  }
  else if (current_menu == show_current_time) {
    current_menu = show_current_temp;
  }
}

// обработка нажатия кнопки set
void process_set() {
  // обработка выбора элементов меню
  if (current_menu == main) {
    // переход в меню настроек
    if (current_menu_element_index == m_settings) {
      current_menu = settings;
      current_menu_element_index = settings_menu_elems[current_settings_menu_index];
    }
    // переход в меню работы
    else if (current_menu_element_index == m_work) {
      if (check_program_exists()) {
        current_menu = work;
        current_menu_element_index = work_menu_elems[current_work_menu_index];
      }
    }
  }
  // обработка внутренних элементов меню настроек
  else if (current_menu == program_redact_index) {
      current_menu = program_redact_temp;
  }
  else if (current_menu == program_redact_temp) {
    current_menu = program_redact_time;
  }
  else if (current_menu == program_redact_time) {
    current_menu = settings;
    current_menu_element_index = settings_menu_elems[current_settings_menu_index];
  }
  else if (current_menu == program_add_index) {
    current_menu = program_add_temp;
  }
  else if (current_menu == program_add_temp) {
    current_menu = program_add_time;
  }
  else if (current_menu == program_add_time) {
    current_menu = settings;
    current_menu_element_index = settings_menu_elems[current_settings_menu_index];
    insert_new_element();
  }
  else if (current_menu == program_remove_index) {
    current_menu = settings;
    current_menu_element_index = settings_menu_elems[current_settings_menu_index];
    remove_element();
  }
  // обработка выбора в меню настроек
  else if (current_menu == settings) {
    if (current_menu_element_index == s_program && check_program_exists()) {
      current_step_show_index = 0;
      current_menu = program_index;
    }
    else if (current_menu_element_index == s_reset) {
      reset_program();
    }
    else if (current_menu_element_index == s_redact && check_program_exists()) {
      current_step_show_index = 0;
      current_menu = program_redact_index;
    }
    else if (current_menu_element_index == s_add && check_program_empty_slots()) {
      current_step_show_index = current_program_len;
      current_menu = program_add_index;
      new_step = {0, 0};
    }
    else if (current_menu_element_index == s_remove && check_program_exists()) {
      current_step_show_index = 0;
      current_menu = program_remove_index;
    }
  }
  // обработка выбора в меню работы
  else if (current_menu == work) {
    if (current_menu_element_index == w_pause) {
      stop_work();
    }
    else if (current_menu_element_index == w_resume) {
      resume_work();
    }
    else if (current_menu_element_index == w_show) {
      current_step_show_index = 0;
      current_menu = show_index;
    }
    else if (current_menu_element_index == w_current) {
      current_menu = show_current_index;
      current_step_show_index = current_step_index;
    }
    else if (current_menu_element_index == w_reset_prog) {
      stop_work();
      current_step_index = 0;
      current_step_time = 0;
    }
    else if (current_menu_element_index == w_prod_pump) {
      clear_water_pump();
    }
  }
}

// обработка нажатия кнопки reset
void process_reset() {
  // обработка выбора элементов меню
  if (current_menu == settings) {
    current_menu = main;
    current_menu_element_index = main_menu_elems[current_main_menu_index];
  }
  else if (current_menu == work) {
    current_menu = main;
    current_menu_element_index = main_menu_elems[current_main_menu_index];
    working = false;
  }
  else if (current_menu == show_index || current_menu == show_temp || current_menu == show_time ||\
  current_menu == show_current_index || current_menu == show_current_temp || current_menu == show_current_desired_temp || current_menu == show_current_time)
  {
    current_menu = work;
    current_menu_element_index = work_menu_elems[current_work_menu_index];
  }
  else if (current_menu == program_index || current_menu == program_temp || current_menu == program_time || current_menu == program_redact_index\
  || current_menu == program_redact_temp  || current_menu == program_redact_time || current_menu == program_add_index || current_menu == program_add_temp\
   || current_menu == program_add_time || current_menu == program_remove_index)
  {
    current_menu = settings;
    current_menu_element_index = settings_menu_elems[current_settings_menu_index];
  }
}

// функция проверки нажатия на кнопки
void check_buttons() {
  if (!btn_lock_timer.elapsed() && btn_lock_timer.active())
    return;

  int btn_cnt = 4;
  bool* buttons[btn_cnt] = {&btn_plus_pressed, &btn_minus_pressed, &btn_set_pressed, &btn_reset_pressed};
  int button_PINs[btn_cnt] = {BTN_PLUS, BTN_MINUS, BTN_SET, BTN_RESET};

  // проверить нажатие на кнопки
  for (int i = 0; i < btn_cnt; i++)
  {
    check_button(buttons[i], digitalRead(button_PINs[i]));
    if (*(buttons[i]))
    {
      btn_lock_timer.setTime(DEFAULT_BTN_LOCK_TIME);
      btn_lock_timer.start();
      return;
    }
  }
}

// функция обработки нажатия на кнопку 
void process_buttons() {
  // при нажатии на любую кнопку зуммер отключается
  if (btn_plus_pressed || btn_minus_pressed || btn_set_pressed || btn_reset_pressed) {
    buzzer_cnt = -1;
  }

  // обработка нажатия на +
  if (btn_plus_pressed)
  {
    process_plus();
    btn_plus_pressed = false;
  }

  // обработка нажатия на -
  if (btn_minus_pressed)
  {
    process_minus();
    btn_minus_pressed = false;
  }

  // обработка нажатия на set
  if (btn_set_pressed)
  {
    process_set();
    btn_set_pressed = false;
  }

  // обработка нажатия на reset
  if (btn_reset_pressed)
  {
    process_reset();
    btn_reset_pressed = false;
  }
}

#pragma endregion button_processing

// добавление нового элемента в программу
void insert_new_element() {
  for (int i = current_program_len - 1; i > current_step_show_index; i--)
  {
    program[i + 1].time = program[i].time;
    program[i + 1].temp = program[i].temp;
  }
  program[current_step_show_index].time = new_step.time;
  program[current_step_show_index].temp = new_step.temp;
  current_program_len += 1;
  current_step_show_index = 0;
}

// удаление элемента программы
void remove_element() {
  for (int i = current_step_show_index; i < current_program_len - 1; i++)
  {
    program[i].time = program[i + 1].time;
    program[i].temp = program[i + 1].temp;
  }
  program[current_program_len - 1].time = 0;
  program[current_program_len - 1].temp = 0;
  current_program_len -= 1;
  current_step_show_index = 0;
}

// продувка помпы для удаления воздушных пробок
void clear_water_pump() {
  stop_work();
  bool temp;
  for (int i = 0; i < CLEAR_REPEATS_CNT; i++) {
    temp = i % 2 == 0;
    pump_relay_switch(temp);
    if (temp)
      delay(PUMP_CLEAR_WORK_TIME);
    else
      delay(PUMP_CLEAR_REST_TIME);
  }
  resume_work();
}

// функция обновления дисплея
void update_display() {
  // точки мигают при нагреве/охлаждении до нужной температуры и просто горят, когда устройство работает
  if (correcting_temperature && working) {
    if (point_blink_tmr.tick())
      point_on = !point_on;
    disp.point(point_on);
  }
  else if (working)
    disp.point(true);
  else {
    disp.point(false);
    point_on = false;    
  }

  if (display_alert) {
    if (millis() < alert_timer + DEFAULT_BTN_LOCK_TIME)
      disp.displayByte(alert_names[alert_index]); 
    else
      display_alert = false;
  }
  // вывод данных во время просмотра и редактирования программы
  else if (current_menu == program_index || current_menu == program_redact_index || current_menu == show_index || current_menu == show_current_index) {
    disp.displayInt(current_step_show_index);
  }
  else if (current_menu == program_temp || current_menu == program_redact_temp || current_menu == show_temp || current_menu == show_current_desired_temp) {
    disp.displayInt(program[current_step_show_index].temp);
  }
  else if (current_menu == program_time || current_menu == program_redact_time || current_menu == show_time) {
    disp.displayInt(program[current_step_show_index].time);
  }
  // вывод оставшегося времени
  else if (current_menu == show_current_time) {
    disp.displayInt(current_step_time);
  }
  // вывод текущей температуры
  else if (current_menu == show_current_temp) {
    disp.displayInt(temperature);
  }
  // вывод данных при создании нового элемента
  else if (current_menu == program_add_index) {
    disp.displayInt(current_step_show_index);
  }
  else if (current_menu == program_add_temp) {
    disp.displayInt(new_step.temp);
  }
  else if (current_menu == program_add_time) {
    disp.displayInt(new_step.time);
  }
  // вывод индекса при удалении элемента
  else if (current_menu == program_remove_index) {
    disp.displayInt(current_step_show_index);
  }
  else
    disp.displayByte(menu_names[current_menu_element_index]);
}

// удалить существующую программу
void reset_program() {
  for (int i = 0; i < MAX_PROGRAM_LEN; i++) {
    program[i].temp = 0;
    program[i].time = 0;
  }
  current_step_index = 0;
  current_program_len = 0;
}

// переключение реле на ТЭН
void heater_relay_switch(bool on) {
  TEN_relay_on = on;
  digitalWrite(TEN_RELAY, on);
}

// переключение реле на насос
void pump_relay_switch(bool on) {
  pump_relay_on = on;
  digitalWrite(PUMP_RELAY, on);
}

// контроль реле
void active_temperature_controll(bool heating, bool on) {
  heater_relay_switch(heating && on);
  pump_relay_switch(!heating && on);
}

// обработка температуры и включение/выключение реле
void process_temp() {
  if (current_step_index == 0 && !reached_step_temp)
  {
    current_step_time = program[current_step_index].time;
  }

  // если для шага установлена максимальная температура
  if (program[current_step_index].temp == TEMP_MAX)
  {
    if ((abs(temperature - program[current_step_index].temp)) <= TEMP_MAX_DELTA) {
      correcting_temperature = false;
      if (reached_step_temp == false)
        reached_step_temp = true;
    }
    else
      correcting_temperature = true;
    active_temperature_controll(true, true);
  }
  // если для шага установлена минимальная температура
  else if (program[current_step_index].temp == TEMP_MIN)
  {
    if ((abs(temperature - program[current_step_index].temp)) <= TEMP_MIN_DELTA) {
      correcting_temperature = false;
      if (reached_step_temp == false)
        reached_step_temp = true;
    }
    else
      correcting_temperature = true;
    active_temperature_controll(false, true);
  }
  // если температура в допустимых пределах от нужной
  else if (abs(temperature - program[current_step_index].temp) <= TEMP_DELTA) {
    correcting_temperature = false;
    if (reached_step_temp == false)
      reached_step_temp = true;      

    active_temperature_controll(true, false);
  }
  // если температура ниже нужной
  else if (temperature < program[current_step_index].temp) {
    correcting_temperature = true;
    active_temperature_controll(true, true);
  }
  // если температура выше нужной
  else if (temperature > program[current_step_index].temp) {
    correcting_temperature = true;
    active_temperature_controll(false, true);
  }

  // обработка таймера шага
  if (reached_step_temp)
  {
    if (current_step_index == 0 && !step_timer.active())
    {
      step_timer.setTime(program[current_step_index].time * 60000);
      step_timer.start();
    }

    current_step_time = step_timer.timeLeft() / 60000 + 1;

    // обработка конца шага
    if (step_timer.tick()) {
      current_step_index += 1;
      current_step_time = program[current_step_index].time;

      // обработка конца программы
      if (current_step_index == current_program_len) {
        start_buzzer(10000);
        stop_work();
      }
      else {
        reached_step_temp = false;
        step_timer.stop();
        step_timer.setTime(current_step_time * 60000);
        step_timer.start();
        start_buzzer(10);
      }
    }
  }
}

// получение значений температуры от градусника
void get_temp() {
  // запрос температуры  
  if (temp_timer.tick()) {
    sensor.requestTemp();
    // проверка успешности чтения и запись температуры в переменную
    if (sensor.readTemp())
      temperature = (int)sensor.getTemp();    
  }
}

// функции управления зуммером
#pragma region buzzer_settings

void start_buzzer(int repeats) {
  buzzer_cnt = repeats;
  buzzer_on = true;
}

void update_buzzer() {
  if (buzzer_timer.tick() && buzzer_cnt >= 0) {
    buzzer_cnt -= 1;
    buzzer_on = !buzzer_on;
  }

  if (buzzer_cnt < 0)
    buzzer_on = false;
  if (buzzer_on)
    digitalWrite(BUZZER, HIGH);
  else
    digitalWrite(BUZZER, LOW);
}

void stop_buzzer() {
  buzzer_cnt = -1;
  buzzer_on = false;
}

#pragma endregion buzzer_settings

// обработка включения светодиодов
#pragma region LED_processing

// функция управления жёлтым светодиодом
void check_yellow_led() {
  // включить, если была нажата кнопка
  if (current_menu == program_add_index || current_menu == program_redact_index || current_menu == program_index || current_menu == show_current_index || current_menu == show_index)
    digitalWrite(YELLOW_LED, HIGH);
  else
    digitalWrite(YELLOW_LED, LOW);
}

// функция управления синим светодиодом
void check_blue_led() {
  if (pump_relay_on)
    digitalWrite(BLUE_LED, HIGH);
  else
    digitalWrite(BLUE_LED, LOW);
}

// функция управления красным светодиодом
void check_red_led() {
  if (TEN_relay_on)
    digitalWrite(RED_LED, HIGH);
  else
    digitalWrite(RED_LED, LOW);
}

#pragma endregion LED_processing

void setup() {
  Serial.begin(9600);

  // настройка пинов управления и вывода
  pinMode(BUZZER, OUTPUT);

  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  pinMode(TEN_RELAY, OUTPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  
  // настройка пинов ввода
  pinMode(BTN_SET, INPUT);
  pinMode(BTN_RESET, INPUT);
  pinMode(BTN_PLUS, INPUT);
  pinMode(BTN_MINUS, INPUT);

  // настройка дисплея
  disp.brightness(7);  // яркость, 0 - 7 (минимум - максимум)
  disp.clear();

  // настройка меню
  current_menu_element_index = m_settings;
  current_menu = main;
}

void loop() {
  // проверка факта нажатия на кнопку
  check_buttons();

  // обработка кнопок
  process_buttons();

  // получить текущую температуру
  get_temp();

  // обработка температуры и подача сигналов на реле
  if (working)
    process_temp();

  // обновить надпись на дисплее
  update_display();

  // обновить зуммер
  update_buzzer();

  // обновление светодиодов
  check_yellow_led();
  check_blue_led();
  check_red_led();
}
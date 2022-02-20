#include <Arduino.h>
#include <pinIO.h>

const uint16_t TIMEOUT_TO_START_GENERATOR = 20; //циклов по 200 мс - время задржки перед запуском генератора
const uint16_t TIMEOUT_GENERATOR_OFF = 120;
const uint16_t TIMEOUT_GENERATOR_START = 12;
const uint16_t TIMEOU_GENERATOR_STABILIZATION = 20; //циклов по 200 мс - время на стабилизацию генератора после запуска

//Входы фаз генератора
//подтянуто аппаратно к Vcc 10k
const uint8_t gen_L1 = 10;
const uint8_t gen_L2 = 9;
const uint8_t gen_L3 = 8;

//Входы фаз основное сети
//подтянуто аппаратно к Vcc 10k
const uint8_t house_L1 = 4;
const uint8_t house_L2 = 5;
const uint8_t house_L3 = 6;

//реле запуска генератора. Пока включено - генератор работает
//запуск генетарора происходит через 23 секунды (+-)
//останавливается генератор примерно через 3 секунды (+-)
const uint8_t rel_Start = A5;

//реле одновременно не включать!
//есть аппаратная защита от одновременного включения
//реле включения напряжения от генератора
const uint8_t rel_Genetator = A0;

//реле включения напряжения от основной сети
const uint8_t rel_HouseNet = A1;

//идикатор Основной сети
const uint8_t LED_HouseNet = 0;

//Индикатор работы генератора
const uint8_t LED_Generator = 1;

//индикатор ошибки
const uint8_t LED_Failure = 2;

// NONE_LINE - нет ни одной фазы, SOME_LINE - 1-2 фазы, ALL_LINE - есть все фазы
enum NET
{
  NONE_LINE = 0,
  SOME_LINE,
  ALL_LINE
};

enum MODE
{
  POW_ON = 0,
  HOUSE_NET_OK,
  GENERATOR_OK,
  GENERATOR_START,
  GENERATOR_STOP,
  GENERATOR_ERROR
};

MODE mode = POW_ON;
///////////////////////////////////////////////////////////////////////////////
//                                 HEADERS
///////////////////////////////////////////////////////////////////////////////

NET scanHouseNet(void);
NET scanGenerator(void);

void _rel_Start_ON(void);
void _rel_Start_OFF(void);
void _rel_Generator_On(void);
void _rel_Generator_OFF(void);
void _rel_HouseNet_On(void);
void _rel_HouseNet_OFF(void);

void _LED_HouseNet_ON(void);
void _LED_HouseNet_OFF(void);
void _LED_Generator_ON(void);
void _LED_Generator_OFF(void);
void _LED_Failure_ON(void);
void _LED_Failure_OFF(void);

///////////////////////////////////////////////////////////////////////////////
//                                 setup
///////////////////////////////////////////////////////////////////////////////
void setup()
{
  // put your setup code here, to run once:

  pinModeFast(gen_L1, INPUT);
  pinModeFast(gen_L2, INPUT);
  pinModeFast(gen_L3, INPUT);
  pinModeFast(house_L1, INPUT);
  pinModeFast(house_L2, INPUT);
  pinModeFast(house_L3, INPUT);

  pinModeFast(rel_Start, OUTPUT);
  pinModeFast(rel_Genetator, OUTPUT);
  pinModeFast(rel_HouseNet, OUTPUT);

  pinModeFast(LED_HouseNet, OUTPUT);
  pinModeFast(LED_Generator, OUTPUT);
  pinModeFast(LED_Failure, OUTPUT);

  _rel_Generator_OFF();
  _rel_HouseNet_OFF();
  _rel_Start_OFF();

  //Тест индикаторов
  _LED_Failure_ON();
  _LED_Generator_ON();
  _LED_HouseNet_ON();

  delay(1500);

  _LED_Failure_OFF();
  _LED_Generator_OFF();
  _LED_HouseNet_OFF();
}

///////////////////////////////////////////////////////////////////////////////
//                                loop
///////////////////////////////////////////////////////////////////////////////
void loop()
{
  uint8_t timer = 0;
  switch (mode)
  {
  case POW_ON: //****************************************** POW_ON *******************************
    if (scanGenerator() == SOME_LINE || scanGenerator() == ALL_LINE)
    {
      mode = GENERATOR_ERROR;
    }
    else
    {
      mode = HOUSE_NET_OK;
    }

    break;

  case HOUSE_NET_OK:                //****************************************** HOUSE_NET_OK *******************************
    if (scanHouseNet() == ALL_LINE) // House net is OK
    {
      _rel_Generator_OFF();
      _rel_Start_OFF();
      _rel_HouseNet_On();

      _LED_Failure_OFF();
      _LED_Generator_OFF();
      _LED_HouseNet_ON();
    }
    else //***************************************************** Проблема с сетью ****************************ы
    {
      timer = 0;
      while (scanHouseNet() != ALL_LINE)
      {
        digitalToggleFast(LED_HouseNet);
        delay(200);
        timer++;
        if (timer > TIMEOUT_TO_START_GENERATOR)
        {
          mode = GENERATOR_START;
          break;
        }
        if (mode == GENERATOR_START)
          break;
      }
    }
    break;

  case GENERATOR_OK: //****************************************** GENERATOR_OK *******************************
    if (scanGenerator() == ALL_LINE)
    {
      _rel_Generator_On();
      _rel_HouseNet_OFF();

      _LED_Generator_ON();
      _LED_Failure_OFF();
      _LED_HouseNet_OFF();

      if (scanHouseNet() == ALL_LINE)
      {
        timer = 0;
        while (scanHouseNet() == ALL_LINE)
        {
          digitalToggleFast(LED_HouseNet);
          delay(200);
          timer++;
          if (timer > 20)
          {
            mode = GENERATOR_STOP;
            break;
          }
          if (mode == GENERATOR_STOP)
            break;
        }
        if (mode == GENERATOR_STOP)
          break;
      }
      if (mode == GENERATOR_STOP)
        break;
    }
    else //*********************************************** генератор заглох
    {
      //выключение генератора, пауза на выключени
      _rel_HouseNet_OFF();
      _rel_Generator_OFF();
      _rel_Start_OFF();

      _LED_Generator_OFF();
      _LED_Failure_ON();
      _LED_HouseNet_OFF();

      for (timer = 0; timer < TIMEOUT_GENERATOR_OFF; timer++)
      {
        digitalToggleFast(LED_Generator);
        delay(200);
      }
      _LED_Generator_OFF();
      _LED_Failure_OFF();

      mode = POW_ON;
    }
    if (mode == GENERATOR_STOP)
      break;

    break;

  case GENERATOR_START: //****************************************** GENERATOR_START *******************************
    _rel_HouseNet_OFF();
    _rel_Generator_OFF();
    _rel_Start_ON();

    _LED_Generator_OFF();
    _LED_Failure_OFF();
    _LED_HouseNet_OFF();

    timer = 0;
    while (!(scanGenerator() == ALL_LINE))
    {
      digitalToggleFast(LED_Generator);

      timer++;

      if (timer > TIMEOUT_GENERATOR_START)
      {
        mode = GENERATOR_ERROR;
        break;
      }
      if (mode == GENERATOR_ERROR)
        break;
      delay(500);
    }
    if (mode == GENERATOR_ERROR)
      break;
    //генератор запущен

    for (timer = 0; timer < 20; timer++)
    {
      digitalToggleFast(LED_Generator);

      delay(200);
    }

    mode = GENERATOR_OK;

    break;

  case GENERATOR_STOP: //****************************************** GENERATOR_STOP *******************************

    _rel_Generator_OFF();
    delay(10);
    _rel_HouseNet_On();
    _rel_Start_OFF();

    _LED_Failure_OFF();
    _LED_HouseNet_ON();

    timer = 0;
    while (scanGenerator() != NONE_LINE)
    {
      digitalToggleFast(LED_Generator);
      timer++;
      delay(200);
      if (timer > TIMEOUT_GENERATOR_OFF)
      {
        mode = GENERATOR_ERROR;
        break;
      }
      if (mode == GENERATOR_ERROR)
        break;
    }
    mode = HOUSE_NET_OK;

    break;

  case GENERATOR_ERROR: //****************************************** GENERATOR_ERROR *******************************
    _rel_Start_OFF();
    _rel_Generator_OFF();
    _rel_HouseNet_OFF();

    _LED_Failure_ON();
    _LED_Generator_ON();
    _LED_HouseNet_OFF();

    while (1)
    {

      digitalToggleFast(LED_Failure);
      digitalToggleFast(LED_Generator);
      delay(500);
    }
    break;

  default:
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////
//                                 scanHouseNet
///////////////////////////////////////////////////////////////////////////////
NET scanHouseNet(void)
{
  NET ret = NONE_LINE;

  if (!digitalReadFast(house_L1) && !digitalReadFast(house_L2) && !digitalReadFast(house_L3))
    ret = ALL_LINE;
  else if (!digitalReadFast(house_L1) || !digitalReadFast(house_L2) || !digitalReadFast(house_L3))
    ret = SOME_LINE;

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//                                 scanGenerator
///////////////////////////////////////////////////////////////////////////////
NET scanGenerator(void)
{
  NET ret = NONE_LINE;

  if (!digitalReadFast(gen_L1) && !digitalReadFast(gen_L2) && !digitalReadFast(gen_L3))
    ret = ALL_LINE;
  else if (!digitalReadFast(gen_L1) || !digitalReadFast(gen_L2) || !digitalReadFast(gen_L3))
    ret = SOME_LINE;

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
void _rel_Start_ON(void)
{
  digitalWriteFast(rel_Start, HIGH);
}

///////////////////////////////////////////////////////////////////////////////
void _rel_Start_OFF(void)
{
  digitalWriteFast(rel_Start, LOW);
}

///////////////////////////////////////////////////////////////////////////////
void _rel_Generator_On(void)
{
  digitalWriteFast(rel_Genetator, HIGH);
}

///////////////////////////////////////////////////////////////////////////////
void _rel_Generator_OFF(void)
{
  digitalWriteFast(rel_Genetator, LOW);
}

///////////////////////////////////////////////////////////////////////////////
void _rel_HouseNet_On(void)
{
  digitalWriteFast(rel_HouseNet, HIGH);
}

///////////////////////////////////////////////////////////////////////////////
void _rel_HouseNet_OFF(void)
{
  digitalWriteFast(rel_HouseNet, LOW);
}

///////////////////////////////////////////////////////////////////////////////
void _LED_HouseNet_ON(void)
{
  digitalWriteFast(LED_HouseNet, HIGH);
}

///////////////////////////////////////////////////////////////////////////////
void _LED_HouseNet_OFF(void)
{
  digitalWriteFast(LED_HouseNet, LOW);
}

///////////////////////////////////////////////////////////////////////////////
void _LED_Generator_ON(void)
{
  digitalWriteFast(LED_Generator, HIGH);
}

///////////////////////////////////////////////////////////////////////////////
void _LED_Generator_OFF(void)
{
  digitalWriteFast(LED_Generator, LOW);
}

///////////////////////////////////////////////////////////////////////////////
void _LED_Failure_ON(void)
{
  digitalWriteFast(LED_Failure, HIGH);
}

///////////////////////////////////////////////////////////////////////////////
void _LED_Failure_OFF(void)
{
  digitalWriteFast(LED_Failure, LOW);
}

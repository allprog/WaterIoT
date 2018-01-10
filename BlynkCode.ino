
/*
 * Projeto - Água IoT (Concurso Hackster Blynk) (Modificado)
 * ---------------------------------------------
 * 
 * 
 * Este projeto permite medir o consumo de água e o fluxo através da internet
 * usando o Sparkfun Blynk Board, a aplicação Blynk e o sensor de fluxo de água.
 * Entre as características deste projeto, as principais são:
 * 
 * - Consumo imediato e medição de fluxo pela Internet
 * - O consumo total é armazenado e comparado com um limite definido
 * pelo usuário (limite de consumo / período de tempo). Então, o usuário pode
 * Verificar e usar a água de forma inteligente e econômica (R$ e ambiente)
 * - Os meterings podem ser calibrados, então
 * este projeto se encaixa em cada lugar (o que é no fluxo do sensor e no limite de pressão, é claro).
 * 
 * De forma geral, é possível:
  * - Leitura de fluxo e consumo
  * - Repor a emissão de compensação
  * - Obter a versão do software (software lançado no Blynk Board)
  * - Calibração inicial e final
 */

/*
 * Includes
 */
#define BLYNK_PRINT Serial    // Comente isso para desabilitar impressões e economizar espaço
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include <ArduinoJson.h>


/*
 * Global definitions
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */
#define SOFTWARE_VERSION      "V1.00"   
#define BREATHING_LIGHT       2  //LED Pin on SparkFun Blynk Board (saida digital - verificar na wemos um pino compativel)
#define YES                   1
#define NO                    0
#define MAX_BUFFER_SIZE       20
#define PIN_SENSOR            5 // entrada digital do sensor de fluxo - GPIO5 D1 na placa
#define PIN_NA                13// entrada de interrupção sensor de nivel alto- GPIO4 D2 na placa
#define PIN_NB                15 // entrada de interrupção sensor de nivel baixo- GPIO0 D3 na placa
#define PIN_BOMBA             14 // controlde da bomba - GPIO14 D5 na placa


//Emula as definições EEPROM
#define EEPROM_BYTES  28    //quantidade de bytes alocados em flash para
                            //simular uma EEPROM. Deve ser entre
                            //4 bytes e 4096 bytes
#define EEPROM_KEY    "PedroBertoleti123"  //Esta é uma chave que será
                                           //armazenada antes dos
                                           //dados relevantes em EEPROM. Isto
                                           //indica se a EEPROM já foi
                                           //inicializada ou não.

#define ADDRESS_EEPROM_KEY               0
#define ADDRESS_PULSES_PER_LITTER        18
#define ADDRESS_CALIBRATION_MODE         22
#define ADDRESS_LIMIT_CONSUMPTION_DEFINE 23
#define ADDRESS_LIMIT_CONSUMPTION        24

// Definições de pinos virtuais (pinos virtuais que serão usados no app blynk)
// Obtendo comandos da aplicação:
//#define VP_CONSUMPTION_RESET         V0  // botão CONS. RESET
//#define VP_SOFTWARE_VERSION          V1  // botão GET VERSION
#define VP_START_CALIBRATION         V2  // botão START
#define VP_END_CALIBRATION           V3  // botão END
#define VP_SET_CONSUMPTION_LIMIT     V4  // slender CONSUMPTION LIMIT
#define VP_MIDDLEWARE                V11

//Enviando dados para o App:
#define VP_CONSUMPTION          V5  // display WATER CONSUMPTION  (L)
#define VP_FLOW_READ                 V6  // display WATER CONSUMPTION AND INSTANT FLOW  (L/H) 
#define VP_FLOW     V7  // display FLOw
#define VP_WARNING_CONSUMPTION_LIMIT V8  // display WATER CONSUMPTION STATUS
#define VP_BREATHING_LIGHT_APP       V9  //  led LED
#define VP_CALIBRATION_IS_OK         V10 // verificar

//JSON class instância
DynamicJsonBuffer jsonBuffer;


/*
 * Variáveis globais
 */

// Configurações do Blynk
char BlynkAuth[] = "94abcf7017af4c02b0795294920c78a7"; //escreva aqui sua chave de autenticação Blynk (auth token)
char WiFiNetwork[] = "CasasBaia";  //escreva aqui o SSID da rede Wi-Fi O Sparkfun Blynk Board conectará
char WiFiPassword[] = "minhacasa"; //escreva aqui a senha da rede Wi-Fi O Sparkfun Blynk Board conectará

//Variaveis gerais
long SensorPulseCounter;
float CalculatedFlow;
float CalculatedConsumption;
long LimitOfConsumption;
char SoftwareVersion[] = SOFTWARE_VERSION; 
long PulsosPerLitter;   //contém quantos pulsos de sensor indicam 1L de consumo
char CalibrationMode;   //indica se a placa está no modo de calibração
char LimitOfConsumptionDefined;  //indica se um limite de consumo de água é definido
unsigned long LastMillis;   //used to time-slice processing (verificar)
//Usado para o tempo de processamento de corte ou usado para a fração de tempo de processamento 
char VarToggle;   //it controls breathing light toggle(ele controla alternar a luz de respiração)
char EEPROMKeyRead[17];
unsigned long ultimaligacao; //processar a fatia de tempo de ativação da bomba

/*
 * Prototypes
 */
void CountSensorPulses(void);
void ResetCountSensorPulses(void);
void CalculateFlowAndConsumption(void);
void SendFlowAndConsumptionToApp(void);
char VerifyLimitOfConsumption(void);
void ShowDebugMessagesInfoCalculated(void);
void TurnOnSensorPulseCounting(void);
void TurnOffSensorPulseCounting(void);
void ControlBreathingLight(void);
void StartEEPROMAndReadValues(void);
long EEPROMReadlong(long address);
void EEPROMWritelong(int address, long value);
void iniciomonitoranivel (void);
void ligabomba (void);
void desligabomba (void);
/*
 * Implementation / functions
 */

// A seguir está a gestão do pino virtual. Estes são como eventos,
// então eles não aparecem nas declarações do protótipo

// Evento: reiniciar o consumo
BLYNK_WRITE(VP_CONSUMPTION_RESET) 
{
    int StatusButton;

    StatusButton = param.asInt();

    //assegure-se de que ele seja executado somente quando o botão virtual estiver no estado on
    if (StatusButton)
    {
        CalculatedConsumption = 0.0;
        Serial.println("[EVENT] O consumo total de água foi redefinido");
    }
}





void uptable(float pass[23])
{

  //Blynk.virtualWrite(V0, "clr");
  //Blynk.virtualWrite(V1, "clr");
  /* dias da semana*/
  /*Blynk.virtualWrite(V0, "add", 0, "Seg", "0");
  Blynk.virtualWrite(V0, "add", 1, "Ter", "0");
  Blynk.virtualWrite(V0, "add", 2, "Qua", "0");
  Blynk.virtualWrite(V0, "add", 3, "Qui", "0");
  Blynk.virtualWrite(V0, "add", 4, "Sex", "0");
  Blynk.virtualWrite(V0, "add", 5, "Sab", "0");
  Blynk.virtualWrite(V0, "add", 6, "Dom", "0");
*/
  /*hora*/
  Serial.print("[Atualizando tabela]:");
  
  for (int i = 0; i<=23;i++)
  {
    Blynk.virtualWrite(V1, "update", i, i, pass[i]);
    Serial.println(pass[i]);
  }
   Blynk.run();

}


// Evento: Consultar valores na middleware
BLYNK_WRITE(VP_MIDDLEWARE) 
{
  String webhookdata = param.asStr();
  JsonObject& root = jsonBuffer.parseObject(webhookdata);
  float time = root[String(1)];
  //Blynk.virtualWrite(V0, "update", 0, webhookdata, webhookdata);
  Serial.print("Retorno do midlleware:");
  Serial.println(webhookdata);
  Serial.print("Retorno de 0: ");
  Serial.println(time);
  float temp[23];
  for (int i = 0; i<=23; i++)
  {
    
    //String v = String(i);
    float a = root[String(i)];
    temp[i] = a;
    //Serial.println(a);
  }
  uptable(temp);

}



//Event: enviar versão de software
/*BLYNK_WRITE(VP_SOFTWARE_VERSION) 
{
    int StatusButton;
    char SoftwareVersion[]=SOFTWARE_VERSION;

    StatusButton = param.asInt();

    //assegure-se de que ele seja executado somente quando o botão virtual estiver no estado on
    if (StatusButton)
    {
        Blynk.virtualWrite(VP_SOFTWARE_VERSION_READ, SoftwareVersion);    
        Serial.println("[EVENT] A versão do software foi enviada");
    }
}*/

//Evento: iniciar a calibração
BLYNK_WRITE(VP_START_CALIBRATION) 
{
    int StatusButton;

    StatusButton = param.asInt();

    //assegure-se de que ele seja executado somente quando o botão virtual estiver no estado on
    if (StatusButton)
    {
        CalibrationMode = YES;
        PulsosPerLitter = 0;
        ResetCountSensorPulses();
        Serial.println("[EVENT] A calibração do sensor começou");
    }
}

//Evento: calibração final
BLYNK_WRITE(VP_END_CALIBRATION) 
{
    int StatusButton;

    StatusButton = param.asInt();

    //assegure-se de que ele seja executado somente quando o botão virtual estiver no estado on
    if (StatusButton)
    {
        PulsosPerLitter = SensorPulseCounter;
        CalibrationMode = NO;    
        
        //Escreva o resultado da calibração para eeprom
        EEPROMWritelong(ADDRESS_PULSES_PER_LITTER,PulsosPerLitter);
        EEPROMWritelong(ADDRESS_CALIBRATION_MODE,CalibrationMode);
        
        ResetCountSensorPulses();
        Serial.print("[EVENT] A calibração do sensor terminou. Fator de calibração: ");
        Serial.print(PulsosPerLitter);
        Serial.println(" pulses / litter");
        Serial.println("[EEPROM] Valores de calibração armazenados na EEPROM.");
    }
}

//Evento: limite de consumo de água definido
BLYNK_WRITE(VP_SET_CONSUMPTION_LIMIT) 
{
    int LimitConsumption;

    Serial.print("[EVENT] Limite do consumo de água agora está definido em ");
    Serial.print(LimitOfConsumption);
    Serial.println(" l");

    LimitConsumption = param.asInt();    
    LimitOfConsumptionDefined = YES;    
    LimitOfConsumption = (long)(LimitConsumption);

    //escrever limite de consumo para eeprom
    EEPROMWritelong(ADDRESS_LIMIT_CONSUMPTION_DEFINE,LimitOfConsumptionDefined);
    EEPROMWritelong(ADDRESS_LIMIT_CONSUMPTION,LimitOfConsumption);
    Serial.println("[EEPROM]Limite o consumo de água armazenado na EEPROM.");
}

//Function:contam pulsos de sensor de fluxo de água
//Params: none
//Return: none 
void CountSensorPulses(void)
{
    SensorPulseCounter++;      
}

//Function: Reiniciar o contador de pulsos do sensor de fluxo de água
//Params: none
//Return: none 
void ResetCountSensorPulses(void)
{  
    SensorPulseCounter = 0;
}

//Function: calcular fluxo instantâneo de água e consumo
//Params: none
//Return: none 
void CalculateFlowAndConsumption(void)
{
    //esta função permite cálculos somente se o sensor já estiver calibrado
    if ((CalibrationMode == YES) || (PulsosPerLitter == 0))
        return;    

    //o cálculo começa!    
    CalculatedFlow = (float)((float)SensorPulseCounter / (float)PulsosPerLitter);  //Fluxo em l/s
 
    CalculatedConsumption = CalculatedConsumption + CalculatedFlow;   //Soma do consumo de 1 segundo
    CalculatedFlow = CalculatedFlow*3600.0;   //Fluxo em l / h     
    Blynk.run();  //atualiza a conexão blynk (keep-alive)
}

//Function: envie o fluxo de água e o consumo calculados para a aplicação Blynk.
//        Além disso, envia o status de calibração
//Params: none
//Return: none 
void SendFlowAndConsumptionToApp(void)
{
    Serial.println(" ");
    
    Serial.print("[LOG] Entrou na funçao de envio");
    
    
    WidgetLED CalibrationLED(VP_CALIBRATION_IS_OK);

    String m = "C: ";
    m += CalculatedConsumption;
    m += " F: ";
    m += CalculatedFlow;
    
    Blynk.virtualWrite(VP_FLOW_READ, m); 
    Blynk.virtualWrite(VP_CONSUMPTION, CalculatedConsumption);     
    Blynk.virtualWrite(VP_FLOW, CalculatedFlow);     
    
    if (PulsosPerLitter == 0)   
        CalibrationLED.off();
    else
        CalibrationLED.on();
                
    Blynk.run();  //atualiza a conexão blynk (keep-alive)
}

//Function: verifique se o consumo de água está abaixo do limite definido
//Params: none
//Return: YES - o consumo de água está acima do limite
//        NO - o consumo de água está abaixo do limite
char VerifyLimitOfConsumption(void)
{
     Blynk.run();  //atualiza a conexão blynk (keep-alive)
     if (LimitOfConsumptionDefined == NO)
         return NO;

     if (CalculatedConsumption > (float)LimitOfConsumption)
         return YES;
     else
         return NO;      
}

//Function: mostram o fluxo calculado de água e o consumo de monitor serial
//Params: none
//Return: none
void ShowDebugMessagesInfoCalculated(void)
{
    
    Serial.println(" ");
    
    Serial.print("[FLOW] Fluxo calculado: ");
    Serial.print(CalculatedFlow);
    Serial.println("l/h");
    Serial.print("[CONSUMPTION] Consumo calculado: ");
    Serial.print(CalculatedConsumption);
    Serial.println("l");
    
    Serial.print("[SENSOR] Número de pulsos: ");
    Serial.println(SensorPulseCounter);

    Serial.print("[CAL_STATUS] ");
    if (CalibrationMode == YES)
        Serial.println("YES");
    else
        Serial.println("NO");

    Serial.println("EEPROM KEY: ");

    for(char i=0;i<17;i++)
        Serial.print(EEPROMKeyRead[i]);

    Serial.println(" ");
    Serial.print("Fator de calibração: ");
    Serial.print(PulsosPerLitter);
    Serial.println(" ");
    
    Blynk.run();  //atualiza a conexão blynk (keep-alive)
}

//Function: Ligar a contagem dos pulsos do sensor
//Params: none
//Return: none
void TurnOnSensorPulseCounting(void)
{
    //esta função permite a reposição do contador de pulso somente se o sensor já estiver calibrado
    if (CalibrationMode == NO)
        ResetCountSensorPulses();
    
    //colocar o pino do sensor como interropção (crescente)
    attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), CountSensorPulses, RISING);
    Blynk.run();  //atualiza a conexão blynk (keep-alive)
}

//Function: Ativar contagem dos pulsos do sensores
//Params: none
//Return: none
void TurnOffSensorPulseCounting(void)
{
    detachInterrupt(digitalPinToInterrupt(PIN_SENSOR));
    Blynk.run();  //atualiza a conexão blynk (keep-alive)
}

//Function: controla breathing light
//Params: none
//Return: none
void ControlBreathingLight(void)
{
    WidgetLED BreathingLightApp(VP_BREATHING_LIGHT_APP);

    //se esta placa estiver sob calibração, o led de respiração não pisca

    if (CalibrationMode==YES)
    {
         digitalWrite(BREATHING_LIGHT,HIGH);    
         BreathingLightApp.on();
         Blynk.run();  //atualiza a conexão blynk (keep-alive)        
         return;
    }
    
    VarToggle = ~VarToggle;
    if (!VarToggle)
    {
        digitalWrite(BREATHING_LIGHT,LOW);
        BreathingLightApp.off();
    }
    else    
    {
        digitalWrite(BREATHING_LIGHT,HIGH);    
        BreathingLightApp.on();
    }
    
    Blynk.run();  //atualiza a conexão blynk (keep-alive)        
}

//Function: inicia EEPROM e lê valores. Se não houver dados relevantes, os dados padrão são escritos em EEPROM.
//Params: none
//Return: none
void StartEEPROMAndReadValues(void)
{
    char EEPROMKey[] = EEPROM_KEY;
    char i;

    EEPROM.begin(EEPROM_BYTES); 

    //lê os primeiros 48 bytes (tamanho da chave EEPROM) para verificar se os dados escritos na EEPROM são relevantes.
    //Se sim, carregue-o. Caso contrário, comece dados EEPROM com valores padrão
    for(i=0;i<17;i++)
        EEPROMKeyRead[i] = EEPROM.read(i);
 
    EEPROM.end();
    
    if (memcmp(EEPROMKey,EEPROMKeyRead,17) == 0)
    {
        PulsosPerLitter = EEPROMReadlong(ADDRESS_PULSES_PER_LITTER);
        CalibrationMode = EEPROM.read(ADDRESS_CALIBRATION_MODE);
        LimitOfConsumptionDefined = EEPROM.read(ADDRESS_LIMIT_CONSUMPTION_DEFINE);
        LimitOfConsumption = EEPROMReadlong(ADDRESS_LIMIT_CONSUMPTION);
        Serial.println("[EEPROM] Os dados da EEPROM estão corretos e carregados com sucesso");    
    }
    else
    {  
        for(i=0;i<17;i++)
            EEPROM.write(i, EEPROMKey[i]);

        CalibrationMode = NO;
        LimitOfConsumptionDefined = NO;
        LimitOfConsumption = NO;
        PulsosPerLitter = 0;

        EEPROMWritelong(ADDRESS_PULSES_PER_LITTER,PulsosPerLitter);
        EEPROM.write(ADDRESS_CALIBRATION_MODE, CalibrationMode);
        EEPROM.write(ADDRESS_LIMIT_CONSUMPTION_DEFINE, LimitOfConsumptionDefined);
        EEPROMWritelong(ADDRESS_LIMIT_CONSUMPTION,LimitOfConsumption);
        Serial.println("[EEPROM]Não houve dados relevantes na EEPROM.");    
    }
}

//Function: Leia lojas de valor longo na EEPROM. Reference: http://playground.arduino.cc/Code/EEPROMReadWriteLong
//Params: primeiro endereço de longo valor
//Return:valor longo
long EEPROMReadlong(long address)
{
    EEPROM.begin(EEPROM_BYTES); 

    //Leia os 4 bytes da memória eeprom.
    long four = EEPROM.read(address);
    long three = EEPROM.read(address + 1);
    long two = EEPROM.read(address + 2);
    long one = EEPROM.read(address + 3);

    EEPROM.end();
    
    //Retorne a recomposição por muito tempo usando o bitshift.
    return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

//Function: escreva lojas de valor longo na EEPROM. Reference: http://playground.arduino.cc/Code/EEPROMReadWriteLong
//Params: primeiro endereço de valor longo e valor longo em si
//Return: none
void EEPROMWritelong(int address, long value)
{
    EEPROM.begin(EEPROM_BYTES); 
    
    // Decomposição de um longo a 4 bytes usando bitshift
    byte four = (value & 0xFF);
    byte three = ((value >> 8) & 0xFF);
    byte two = ((value >> 16) & 0xFF);
    byte one = ((value >> 24) & 0xFF);

    //Escreva os 4 bytes na memória eeprom.
    EEPROM.write(address, four);
    EEPROM.write(address + 1, three);
    EEPROM.write(address + 2, two);
    EEPROM.write(address + 3, one);
    EEPROM.end();
}





//Comandos dos sensores de nível e bomba
void iniciomonitoranivel (void)
{
    digitalWrite(PIN_BOMBA, LOW);
    attachInterrupt(digitalPinToInterrupt(PIN_NA), desligabomba, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_NB), ligabomba, FALLING);
}

/*
void fimmonitoranivel (void)
{
    detachInterrupt(digitalPinToInterrupt(PIN_NA));
    detachInterrupt(digitalPinToInterrupt(PIN_NB));
}
*/

void ligabomba (void)
{
  
  if(millis()-ultimaligacao>3000)//esta condição evita que alguma ondulação na caixa ligue a bomba
    digitalWrite(PIN_BOMBA, HIGH);
  
  ultimaligacao = millis();
}

void desligabomba (void)
{
  if(millis()-ultimaligacao>3000) //esta condição evita que alguma ondulação na caixa desligue a bomba
    digitalWrite(PIN_BOMBA, LOW);

  ultimaligacao = millis();
}





void setatabela()
{

  Blynk.virtualWrite(V0, "clr");
  Blynk.virtualWrite(V1, "clr");
  /* dias da semana*/
  Blynk.virtualWrite(V0, "add", 0, "Seg", "0");
  Blynk.virtualWrite(V0, "add", 1, "Ter", "0");
  Blynk.virtualWrite(V0, "add", 2, "Qua", "0");
  Blynk.virtualWrite(V0, "add", 3, "Qui", "0");
  Blynk.virtualWrite(V0, "add", 4, "Sex", "0");
  Blynk.virtualWrite(V0, "add", 5, "Sab", "0");
  Blynk.virtualWrite(V0, "add", 6, "Dom", "0");

  /*hora*/
  Blynk.virtualWrite(V1, "add", 0, "0h", "0");
  Blynk.virtualWrite(V1, "add", 1, "1h", "0");
  Blynk.virtualWrite(V1, "add", 2, "2h", "0");
  Blynk.virtualWrite(V1, "add", 3, "3h", "0");
  Blynk.virtualWrite(V1, "add", 4, "4h", "0");
  Blynk.virtualWrite(V1, "add", 5, "5h", "0");
  Blynk.virtualWrite(V1, "add", 6, "6h", "0");
  Blynk.virtualWrite(V1, "add", 7, "7h", "0");
  Blynk.virtualWrite(V1, "add", 8, "8h", "0");
  Blynk.virtualWrite(V1, "add", 9, "9h", "0");
  Blynk.virtualWrite(V1, "add", 10, "10h", "0");
  Blynk.virtualWrite(V1, "add", 11, "11h", "0");
  Blynk.virtualWrite(V1, "add", 12, "12h", "0");
  Blynk.virtualWrite(V1, "add", 13, "13h", "0");
  Blynk.virtualWrite(V1, "add", 14, "14h", "0");
  Blynk.virtualWrite(V1, "add", 15, "15h", "0");
  Blynk.virtualWrite(V1, "add", 16, "16h", "0");
  Blynk.virtualWrite(V1, "add", 17, "17h", "0");
  Blynk.virtualWrite(V1, "add", 18, "18h", "0");
  Blynk.virtualWrite(V1, "add", 19, "19h", "0");
  Blynk.virtualWrite(V1, "add", 20, "20h", "0");
  Blynk.virtualWrite(V1, "add", 21, "21h", "0");
  Blynk.virtualWrite(V1, "add", 22, "22h", "0");
  Blynk.virtualWrite(V1, "add", 23, "23h", "0");


   Blynk.run();

}

void uptable()
{
  //f
}

//Setup function. Aqui, os inits são feitos!



void setup() 
{
    Serial.begin(115200);

    //inicia EEPROM e carrega valores guardados
    StartEEPROMAndReadValues();
    
    //variáveis init
    ResetCountSensorPulses();
    CalculatedFlow = 0.0;
    CalculatedConsumption = 0.0;
    ultimaligacao = 0;
      
    //configure o pino do sensor como entrada
    pinMode(PIN_SENSOR, INPUT);
    pinMode(PIN_NB, INPUT);
    pinMode(PIN_NA, INPUT);

    //configure o pino de controle da bomba
    pinMode(PIN_BOMBA, OUTPUT);

    //configurar o pino de breathing light
    pinMode(BREATHING_LIGHT,OUTPUT);
    
    //inicia monitoramente de nível de água da caixa
    iniciomonitoranivel();

    //inicializar Blynk
    Blynk.begin(BlynkAuth, WiFiNetwork, WiFiPassword);
    
    //Comece a contar os pulsos do sensor
    TurnOnSensorPulseCounting();
    VarToggle = 0;
    LastMillis = millis();

    setatabela();

}



//main loop / main program
void loop() 
{   
    Blynk.run();    
    
    if ((millis() - LastMillis) >= 1000)
    {
       // para evitar erros de cálculo, a aquisição do pulso do sensor está desativada      
        TurnOffSensorPulseCounting();
        
        //calcular o consumo de água e fluxo
        CalculateFlowAndConsumption();
        Blynk.run();    
        
        //enviar valores calculados para a aplicação Blynk
        SendFlowAndConsumptionToApp();       
        Blynk.run();    

        uptable();       
        Blynk.run();   
        
        //depurar mensagens
        ShowDebugMessagesInfoCalculated();
        Blynk.run();    
        
        //verifique se o consumo de água está abaixo do limite definido
        if (VerifyLimitOfConsumption() == YES)
             Blynk.virtualWrite(VP_WARNING_CONSUMPTION_LIMIT, 1023); 
        else
             Blynk.virtualWrite(VP_WARNING_CONSUMPTION_LIMIT, int(CalculatedConsumption/LimitOfConsumption*1024-1)); 
        Blynk.run();    
        
         //redefinir o processamento do tempo-fatia
        LastMillis = millis();    
        Blynk.run();    
        
        //O cálculo e o envio de dados estão completos! A aquisição do pulso do sensor começa novamente        
        TurnOnSensorPulseCounting();
        Blynk.run();

        Blynk.virtualWrite(VP_MIDDLEWARE, 1);
        Blynk.run();
        
        
        //projeto de controle breathing light
        ControlBreathingLight();   
        Blynk.run();    
    }
}

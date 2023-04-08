# Önálló laboratórium - IoT adatgyűjtés, adatelemzés

## Észrevételek a fejlesztés során, amik majd dokumentálhatóak:

### Byteorder
A szenzor (MPU6050) a szükséges gyorsulásértékeket 6 regiszterben tárolja. X, Y, és Z tengely irányában így 16 bites számok formájában mér gyorsulást. Sorrendhelyesen a mérési adatokat az alábbi módon lehet kiolvasni:

`[ACC_X_MSB][ACC_X_LSB][ACC_Y_MSB][ACC_Y_LSB][ACC_Z_MSB][ACC_Z_LSB]`

Ilyenkor tehát az első beérkező bájt az X tengely menti gyorsulás mért mértékének a felső 8 bitje, a második az alsó 8 bit, stb.

Viszont az ESP32 LSB módon dolgozik, így az alábbi (egyszerűsített) kódrészlet hibás:
```c
uint8_t buf[6] = {0};
read_data(buf);
int16_t *res = (void*)buf;
int16_t x = res[0];
```

Ilyenkor ugyanis az `int16_t` érték a szenzor által használt bájtsorrent alapján kerül kiolvasásra a processzor regiszterébe, tehát `[ACC_X_LSB][ACC_X_MSB]` sorrendben, mely hibás érték megjelenítéséhez vezet. 

Megoldás erre az, hogy meg kell cserélni a bájtsorrendeket. Erre egy nagyon egyszerű megoldást választottam: Egy új bájt tömbbe már a megfelelő sorrendben helyeztem el a bájtokat, és *arra* mutattam `int_16t` pointerrel. A további számításokhoz `float` típus szükséges, de ez már triviális. 

### FreeRTOS Task prioritás és Interrupt kezelés

A FreeRTOS önmagában képes arra, hogy különböző prioritású taskok között magától váltson. Ez nem történik meg viszont azonnal. Létezik egy `configCPU_TICK_HZ` nevű define, mely segítségével megadható, mennyi "tick" után történjen egy FreeRTOS Timer Interrupt. Ez felel a Context Switching megvalósításáért többek között. Ha a processzor órajele 80MHz és ennek a define-nak az értéke 100, akkor minden (80MHz/100) = 800.000 órajelciklus után ***mindenképpen*** történik egy ilyen interrupt (vagy ha úgy tetszik, [minden 1ms után](https://onlinedocs.microchip.com/pr/GUID-F3CEAE3B-C3C1-4B92-B031-4E07B8ACCD81-en-US-3/index.html?GUID-F9AFE28C-0CAB-4DF6-985F-B5844B6B9AE0)). Ekkor az ütemező megkeresi a legnagyobb prioritásu, Ready állapotban lévő taskot, és arra vált. 

Probléma volt a kódomban, hogy a szenzor felől érkező interruptot kezelő ISR (Interrupt Service Routine) bár lefutott, a task, mely az I2C kommunikációért magáért lenne felelős nem minden esetben kapta meg a prioritást - még úgy sem, hogy prioritása a legmagasabb volt.

A problémára való első megoldásom a következő volt: Hogyha `configCPU_TICK_HZ` értéke 1000, akkor pontosan minden millisecundumban történik Context Switching, és mivel a I2C kommunikáció legnagyobb prioritású, így az előnyt fog élvezni mindennel szemben az ütemező alapján. A megoldás működött is, látszólag. Egy GPIO kimenetre kötött LED villogása stabilizálódott, az alapján valóban közen (1ms+epsilon) idő alatt összegyűlt 1024 mérési adat. 

Viszont e megoldással a probléma a Tick Interrupt gyakori futása, ami így minden 1ms után bekövetkezik. A dokumentáció alaposabb átvizsgálása során úgy láttam, hogy van egy másik megoldás: `portYIELD_FROM_ISR( xHigherPriorityTaskWoken );`.

Ez a függvény az ISR-be kerül, amely így így néz ki:
```c
static void IRAM_ATTR gpio_isr_handler(void* arg) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xInterruptSemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
```

Mivel az interrupt minden fölött áll, így bármilyen prioritású taskot megállít. `xSemaphoreGiveFromISR()` függvény aktiválja a Binary Semaphore-t, melynek jelére az I2C kommunikációért felelős task vár. Ez a függvény meg tudja mondani azt, hogy a Semaphore beállításával Blocked állapotból Ready állapotba került-e egy olyan task, melynek prioritása nagyobb, mint az ISR által megszakított, éppen futó (Running állapotban lévő) task prioritása. Amennyiben igen, az ütemező a magasabb prioritású taskot fogja futtatni, *de csak abban az esetben*, amennyiben ezt a `portYIELD_FROM_ISR()` makrófüggvénnyel megmondjuk. Enélkül, a FreeRTOS ütemező folytatja a megszakított task futását addig, amíg a számára kiosztott Time Slice le nem telik. Hogyha épp a Fourier transzformációt végző task került megszakításra, az folytatódik, és kommunikáció nem történik. A megadott függvény hívásával, mivel az I2C kommunikációt adó függvény magasabb prioritású, az ütemező azt indítja el.

### Analóg mikrofon

Analóg mikrofont (is) kaptam. 3,3V-ról táplálom, a kimeneti OUT lábán (Vdd/2), azaz olyan 1,65V jön ki teljes csend esetén
Az ESP32 ADC-jének referencia feszültsége (Vref) olyan 1-1,2V között van, kalibrációtól függően

Egész idáig `ADC_ATTEN_DB_0` beállítással mértem, `100 mV ~ 950 mV` között, ami hát... Már a 1,65V is jóval nagyobb ennél
`ADC_ATTEN_DB_11` beállítás segített egész jól, `150 mV ~ 2450 mV` az intervallum így, de hát ez sem a legjobb, *de* legalább rájöttem, hogy mi is jön vissza a mikrofonból: A teljes hullám. (Vdd/2) fölött a pozitív, az alatt meg a negatív fele

### FFT függvény absztrahálása

Az FFT függvénynek szüksége van az adathalmazra (melyben a transzformálandó értékek vannak), valamint e tömbnek a hossza. 


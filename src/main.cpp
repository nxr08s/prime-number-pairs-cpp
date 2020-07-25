/*
*	Устинов А.В. 15.03.2019 19:21
*	Программа для вычисления пар простых чисел в диапазоне от 1ккк до 2ккк.
*	Используется многопоточность для расчета и вывода информации.
*
*	Алгоритм:
*		1. Расчитываются в одном потоке первичный массив простых чисел (до sqrt(2,000,000,000)).
*		2. Используя расчитанные значения, на всех потоках машины рассчитываются простые числа от 1ккк до 2ккк.
*		3. Используя значения, рассчитанные в шаге 2, находятся пары простых чисел.
*		4. Найденные пары сохраняются в файл "primePairs.txt" на рабочем столе пользователя.
*/

/*
*		Машина 1:
*			ЦП:	 Intel Pentium 3805U (1.9 GHz, 2 CPUs)
*			ОЗУ: 4096 MB (DDR3 1600 MHz)
*			2 потока:	535 секунд.
*
*		Машина 2:
*			ЦП:	 Intel Core i7-7700K (4.2 GHz (4.6 - Boost), 8 CPUs)
*			ОЗУ: 16384 MB (DDR4 2400 MHz)
*			8 потоков:
*
*		Машина 3:
*			ЦП:	 Intel Core i5-4590 (3.3 GHz (3.7 - Boost), 4 CPUs)
*			ОЗУ: 8192 MB (DDR3 1333 MHz)
*			4 потока:
*
*		Машина 4: 
*			ЦП:	 Intel Core i3-2100 (3.1 GHz, 4 CPUs)
*			ОЗУ: 2048 MB (DDR3 1333 MHz)
*			4 потока:
*
*		Машина 5:
*			ЦП:	 AMD Phenom II x4 955 (3.2 GHz (4 GHz - OC), 4 CPUs)
*			ОЗУ: 8192 MB (DDR3 1333 MHz) (1600 MHz - OC)
*			4 потока:	868 секунд.
*/

#include "windows.h"
#include "process.h"
#include "stdio.h"
#include "time.h"

int NUMBER_OF_THREAD = 1;	// кол-во логических процессоров в системе

int calculatePrimaryArray(int*);			// функция расчета простых чисел до sqrt(2,000,000,000) : 44,722.
void processThread(void*);					// поток обработки простых числел.
void infoThread(void*);						// вывод состояния обработки в консоль.
int savePairsInFile(char*, int*, int**);	// функция вывода пар простых чисел в файл

// структура с данными для каждого потока обработки
typedef struct _processThreadData {
	int *primaryArray;		// указатель на массив сгенерированных простых чисел до sqrt(2,000,000,000) : 44,722.
	int *arrayPtr;			// указатель на массив куда сохранять.
	int *arraySize;			// размер массива куда сохранять простые числа.
	int firstNumber;		// первое число для проверки.
	int lastNumber;			// последнее число для проверки.
	int currentNumber;		// текущее проверяемое значение (нужно для вывода статуса в консоль потоком infoThread()).
} processThreadData;

// структура с данными для потока вывода инфрмации в консоль
typedef struct _infoThreadData {
	BOOL progStatus;				// текущее состояние вычислений (1 - в работе, 0 - завершено).
	processThreadData *threadsData;	// указатель на массив с данными каждого потока
} infoThreadData;

int main() {
	// время начала работы программы
	time_t startTime = time(NULL);

	// получение кол-ва логических процессоров в системе
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	NUMBER_OF_THREAD = sysInfo.dwNumberOfProcessors;

	// хэндлы потоков вычисления и потока вывода информации
	HANDLE *threadsHandles = (HANDLE*)malloc(NUMBER_OF_THREAD * sizeof(HANDLE));
	HANDLE infoThreadHandle;

	// массив с данными каждого потока вычисления и данные потока вывода информации
	processThreadData *procThreadsData = (processThreadData*)malloc(NUMBER_OF_THREAD * sizeof(processThreadData));
	infoThreadData infoData;

	// вычисление первичного массива с простыми числами
	int primaryPrimeNumbers[5000];
	int primaryArraySize = calculatePrimaryArray(primaryPrimeNumbers);

	// массивы с вычисленными каждым потоком простыми числами, массив с их кол-вом в каждом потоке и массив с кол-вом проверяемых чисел каждым потоком
	int **threadsResultsArrays = (int**)malloc(NUMBER_OF_THREAD * sizeof(int*));
	int *threadsResultsArraysSizes = (int*)malloc(NUMBER_OF_THREAD * sizeof(int));
	int *numbersPerThread = (int*)malloc(NUMBER_OF_THREAD * sizeof(int));

	printf("Generated %d prime numbers.\nCalculating will start on %d threads.\n", primaryArraySize, NUMBER_OF_THREAD);

	// расчет кол-ва проверяемых чисел каждым потоком и корректировка их с учетом разницы скорости вычисления
	for (int i = 0; i < NUMBER_OF_THREAD; i++)
		numbersPerThread[i] = 1000000000 / NUMBER_OF_THREAD;
	for (int i = NUMBER_OF_THREAD - 1; i > 0; i--) {
		int part = numbersPerThread[i] * 0.09;
		numbersPerThread[i] -= part;
		numbersPerThread[i - 1] += part;
	}

	// заполение данных каждого потока
	for (int i = 0; i < NUMBER_OF_THREAD; i++) {
		// инициализация массива с вычисленными простыми числами и задание ему начального размера
		threadsResultsArraysSizes[i] = 0;
		threadsResultsArrays[i] = (int*)malloc(50000000 / (NUMBER_OF_THREAD / 2) * sizeof(int));

		// передача указателей на массивы куда сохранять вычисленные числа
		procThreadsData[i].arrayPtr = threadsResultsArrays[i];
		procThreadsData[i].arraySize = &threadsResultsArraysSizes[i];

		// передача массива с вычисленными простыми числами
		procThreadsData[i].primaryArray = primaryPrimeNumbers;

		// задание границ для поиска простых чисел потоком
		if (i == 0)		procThreadsData[i].firstNumber = 1000000000;
		else			procThreadsData[i].firstNumber = procThreadsData[i - 1].lastNumber;
		procThreadsData[i].lastNumber = procThreadsData[i].firstNumber + numbersPerThread[i];
	}
	// корректировка чтобы не терять последние несколько элементов
	procThreadsData[NUMBER_OF_THREAD - 1].lastNumber = 2000000000;

	// запуск потоков вычисление простых чисел
	for (int i = 0; i < NUMBER_OF_THREAD; i++)
		threadsHandles[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)processThread, &procThreadsData[i], 0, NULL);

	// инициализация данных для потока вывода информации и запуск его
	infoData.threadsData = procThreadsData;
	infoData.progStatus = TRUE;
	infoThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)infoThread, &infoData, 0, NULL);

	// удаление временного массива для вычисления диапазонов для поиска простых чисел
	free(numbersPerThread);

	// ожидание завершения всех потоков и завершение потока вывода информации
	WaitForMultipleObjects(NUMBER_OF_THREAD, threadsHandles, TRUE, INFINITE);
	infoData.progStatus = FALSE;
	WaitForSingleObject(infoThreadHandle, INFINITE);

	// вывод кол-ва найденных простых чисел в диапазоне от 1ккк до 2ккк
	int summary = 0;
	for (int i = 0; i < NUMBER_OF_THREAD; i++)
		summary += threadsResultsArraysSizes[i];
	printf("Calculated %d prime numbers within 1kkk to 2kkk.\n", summary);

	// сохранение всех пар простых чисел в файл primePairs.txt на рабочем столе
	char filePath[128];
	GetEnvironmentVariable("USERPROFILE", filePath, sizeof(filePath));
	strcat_s(filePath, sizeof(filePath), "\\Desktop\\primePairs.txt");
	int numberOfPairs = savePairsInFile(filePath, threadsResultsArraysSizes, threadsResultsArrays);
	printf("%d pairs were saved in %s.\n", numberOfPairs - 1, filePath);

	// вывод времени работы программы
	printf("Time left: %d seconds.\n", time(NULL) - startTime);

	system("pause");
	return 0;
}

// вычисление первичного массива простых чисел
int calculatePrimaryArray(int* primaryArray) {
	int primeNumbersCounter = 1;	// кол-во чисел в первичном массиве
	primaryArray[0] = 2;
	BOOL divisionDetectedFlag;	// флаг устанвливается в TRUE, если произошло деление без остатка на одно из простых чисел


	for (int numberToCheck = 3; numberToCheck * numberToCheck < 2000000000 + 1; numberToCheck += 2) {
		divisionDetectedFlag = FALSE;
		int numberToCheckRoot = sqrt(numberToCheck);
		for (int i = 0; primaryArray[i] < numberToCheckRoot + 1 && i < primeNumbersCounter; i++)
			if (numberToCheck % primaryArray[i] == 0) {
				divisionDetectedFlag = TRUE;
				break;
			}
		if (divisionDetectedFlag == FALSE)
			primaryArray[primeNumbersCounter++] = numberToCheck;
	}
	primaryArray[primeNumbersCounter] = INT_MAX;	// костыль для избавления от проверок на выход за границы массива
	return primeNumbersCounter;				
}

// поток вычисления простых чисел
void processThread(void* param) {
	processThreadData *data = (processThreadData*)param;
	BOOL divisionDetectedFlag;	// флаг устанвливается в TRUE, если произошло деление без остатка на одно из простых чисел

	for (data->currentNumber = data->firstNumber + ((data->firstNumber + 1) % 2); data->currentNumber < data->lastNumber; data->currentNumber += 2) {
		divisionDetectedFlag = FALSE;
		int currentNumberRoot = sqrt(data->currentNumber) + 1;
		for (int i = 0; data->primaryArray[i] < currentNumberRoot; i++)
			if (data->currentNumber % data->primaryArray[i] == 0) {
				divisionDetectedFlag = TRUE;
				break;
			}
		if (divisionDetectedFlag == FALSE)
			data->arrayPtr[(*data->arraySize)++] = data->currentNumber;
	}
	_endthreadex(0);
}

// поток вывода процесса вычислений в консоль
void infoThread(void* param) {
	infoThreadData *data = (infoThreadData*)param;

	char tempOutputString[16];	// временная строка для хранения состояния оного потока
	char outputStr[512];		// полная строка для вывода

	while (data->progStatus) {
		outputStr[0] = '\0';
		for (int i = 0; i < NUMBER_OF_THREAD; i++) {
			sprintf_s(tempOutputString,sizeof(tempOutputString),"%.2f%% ", 
				(double)(data->threadsData[i].currentNumber - data->threadsData[i].firstNumber) / 
				(double)(data->threadsData[i].lastNumber - data->threadsData[i].firstNumber) * 100
			);
			strcat_s(outputStr, sizeof(outputStr), tempOutputString);
		}
		printf("\r%s", outputStr);
		Sleep(500);
	}
	printf("\n");
	_endthreadex(0);
}

// сохранение пар простых чисел в файл
int savePairsInFile(char* fname, int* arraysSizes, int** arrays) {
	FILE* file;
	int pairsCounter = 1;
	
	// открытие файла и проверка на открытие
	fopen_s(&file, fname, "w");
	if (file == NULL)
		return 0;

	// поиск пар и сохранение их в файл
	for (int i = 0; i < NUMBER_OF_THREAD; i++) {
		// проверка стыка 2-ух массивов с найденными простыми числами
		if (i && arrays[i][0] - arrays[i - 1][arraysSizes[i - 1] - 1] == 2)
			fprintf(file, "%d.\t%d %d\n", pairsCounter++, arrays[i - 1][arraysSizes[i - 1] - 1], arrays[i][0]);
		// проверка в пределах массивов
		for (int j = 0; j < arraysSizes[i] - 1; j++)
			if (arrays[i][j + 1] - arrays[i][j] == 2)
				fprintf(file, "%d.\t%d %d\n", pairsCounter++, arrays[i][j], arrays[i][j + 1]);
	}

	// вывод кол-ва пар в файл и закрытие файла
	fprintf(file, "\n%d pairs.\n", pairsCounter - 1);
	fclose(file); 
	return pairsCounter;
}
#include "stdafx.h"
#include "mpi.h"

#define NUM_ROWS 5
#define NUM_COLS 10

#define DATA_COUNT_TAG 1
#define DATA_TO_CALCULATION_TAG 2
#define DATA_FROM_CALCULATION_TAG 3

double MatrixA[NUM_ROWS][NUM_COLS];
double MatrixB[NUM_ROWS][NUM_COLS];
double MatrixResult[NUM_ROWS][NUM_COLS];

int main(int argc, char * argv[])
{
	int rank, size;
	double time, timeStartParallelMatrixSum, timeStopParallelMatrixSum, timeStartSerialMatrixSum, timeStopSerialMatrixSum;
	MPI_Status status;

	int DataCount;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	
	if (rank == 0)
	{
		// Инициализация массивов
		printf("---rank %d initialize MatrixA...\n",rank);
		time = MPI_Wtime();
		for (int i = 0; i < NUM_ROWS; i++)
		{
			for (int j = 0; j < NUM_COLS; j++)
			{
				MatrixA[i][j] = i + j;
				//printf("%.2f\t", MatrixA[i][j]);
			}
			//printf("\n");
		}
		printf("---rank %d initialized MatrixA for %f seconds\n", rank, MPI_Wtime() - time);



		printf("---rank %d initialize MatrixB...\n", rank);
		time = MPI_Wtime();
		for (int i = 0; i < NUM_ROWS; i++)
		{
			for (int j = 0; j < NUM_COLS; j++)
			{
				MatrixB[i][j] = i * j;
				//printf("%.2f\t", MatrixB[i][j]);
			}
			//printf("\n");
		}
		printf("---rank %d initialized MatrixB for %f seconds\n", rank, MPI_Wtime() - time);
		////// End Инициализация массива



		timeStartParallelMatrixSum = MPI_Wtime();
		// Отсылаем строки узлам для расчета		
		int RowsToNodes = NUM_ROWS / size;//Число строк на один узел кроме нулевого
		printf("RowsToNodes=%i\n", RowsToNodes);
		int RowsToNode0 = NUM_ROWS % size;//Число строк на нулевой узел 
		if (RowsToNode0 != 0)
		{
			RowsToNodes++;
			RowsToNode0 = NUM_ROWS - (size - 1)*RowsToNodes;
		}
		else
		{
			RowsToNode0 = RowsToNodes;
		}
		printf("RowsToNode0=%i\n", RowsToNode0);
		printf("NumRows=%i Sum=%u\n", NUM_ROWS, RowsToNodes*(size-1)+RowsToNode0);

		
		for (int node = 1; node < size; node++)
		{
			double *DataToCalculation = new double[NUM_COLS*RowsToNodes*2];
			int i = 0;
			for (int row = (node-1)*RowsToNodes;row<(node - 1)*RowsToNodes+ RowsToNodes;row++)
			{			
				for(int col=0;col<NUM_COLS;col++)
				{					
					DataToCalculation[i] = MatrixA[row][col];
					//printf("DataToCalculation[%d] = MatrixA[%d][%d] = %.2f \n ", i, row, col, MatrixA[row][col]);
					i++;
					DataToCalculation[i] = MatrixB[row][col];
					//printf("DataToCalculation[%d] = MatrixB[%d][%d] = %.2f \n ", i, row, col, MatrixB[row][col]);
					i++;				

				}
			}

			// Посылаем сообщение с объёмом массива
			DataCount = 2 * RowsToNodes * NUM_COLS;
			MPI_Send(&DataCount, 1, MPI_INT, node, DATA_COUNT_TAG, MPI_COMM_WORLD);
			printf("rank %d sended message DATA_COUNT_TAG %d to %d\n", rank, DataCount, node);

			// Посылаем сообщение с данными для расчета
			MPI_Send(DataToCalculation, DataCount, MPI_DOUBLE, node, DATA_TO_CALCULATION_TAG, MPI_COMM_WORLD);
			printf("rank %d sended message DATA_TO_CALCULATION_TAG %.2f to %d\n", rank, DataToCalculation[0], node);						
		}

		////// Вычисляем свою часть данных //////////
		for (int row = (size - 1)*RowsToNodes; row<(size - 1)*RowsToNodes + RowsToNode0; row++)
		{
			for (int col = 0; col<NUM_COLS; col++)
			{
				MatrixResult[row][col] = MatrixA[row][col]+MatrixB[row][col];				
			}
		}
		/////////////////////////////////////////////

		// Получаем от узлов результаты расчетов		
		double *nodeResults = new double[DataCount / 2];
		
		for(int node=1;node<size;node++)
		{
			double *nodeResults=new double[DataCount/2];
			MPI_Recv(nodeResults, DataCount/2, MPI_DOUBLE, node, DATA_FROM_CALCULATION_TAG, MPI_COMM_WORLD, &status);
			
			int i = 0;
			for (int row = (node - 1)*RowsToNodes; row<(node - 1)*RowsToNodes + RowsToNodes; row++)
			{
				for (int col = 0; col<NUM_COLS; col++)
				{
					MatrixResult[row][col] = nodeResults[i];
					i++;
				}
			}
		}
		timeStopParallelMatrixSum = MPI_Wtime();		
				
		printf("-------------- RESULT PARALLEL ---------------\n");
		for (int i = 0; i < NUM_ROWS; i++)
		{
			for (int j = 0; j < NUM_COLS; j++)
			{				
				printf("%.2f\t", MatrixResult[i][j]);
			}
			printf("\n");
		}
		printf("-------------- END RESULT ---------------\n");



		///// Последовательное суммирование
		timeStartSerialMatrixSum = MPI_Wtime();
		for (int i = 0; i < NUM_ROWS; i++)
		{
			for (int j = 0; j < NUM_COLS; j++)
			{
				MatrixResult[i][j]=MatrixA[i][j]+MatrixB[i][j];
			}			
		}
		timeStopSerialMatrixSum = MPI_Wtime();
		////////////////


		printf("-------------- RESULT SERIAL ---------------\n");
		for (int i = 0; i < NUM_ROWS; i++)
		{
			for (int j = 0; j < NUM_COLS; j++)
			{
				printf("%.2f\t", MatrixResult[i][j]);
			}
			printf("\n");
		}
		printf("-------------- END RESULT ---------------\n");


		printf("RESULTS\nTime parallel matrix sum = %f\n", timeStopParallelMatrixSum - timeStartParallelMatrixSum);
		printf("Time serial matrix sum = %f\n", timeStopSerialMatrixSum - timeStartSerialMatrixSum);
	}
	else
	{
		// Получаем сообщение с объёмом массива
		int DataCount;
		MPI_Recv(&DataCount, 1, MPI_INT, MPI_ANY_SOURCE, DATA_COUNT_TAG, MPI_COMM_WORLD, &status);
		printf("P:%d Got message DATA_COUNT_TAG %d from processor %d \n", rank, DataCount, status.MPI_SOURCE);

		

		//Получаем данные для расчета
		double *recievingData = new double[DataCount];
		MPI_Recv(recievingData, DataCount, MPI_DOUBLE, MPI_ANY_SOURCE, DATA_TO_CALCULATION_TAG, MPI_COMM_WORLD, &status);
		printf("P:%d Got message DATA_TO_CALCULATION_TAG %.2f from processor %d \n", rank, recievingData[0], status.MPI_SOURCE);
		

		// Вычисляем и отправляем результат
		double *resultData = new double[DataCount/2];
		int resultCount = 0;
		for (int i=0;i<DataCount;i=i+2)
		{
			resultData[resultCount] = recievingData[i] + recievingData[i + 1];
			//printf("i=%d %d %d | %.2f + %.2f = %.2f \n ",i,i+1, resultCount,recievingData[i], recievingData[i+1], resultData[resultCount]);
			resultCount++;
		}
						
		MPI_Send(resultData, DataCount / 2, MPI_DOUBLE, 0, DATA_FROM_CALCULATION_TAG, MPI_COMM_WORLD);
		printf("---Node %d sended results to node 0. Count = %d \n ", rank, DataCount / 2);
	}


	MPI_Barrier(MPI_COMM_WORLD);	
	

	MPI_Finalize();
    return 0;
}


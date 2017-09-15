/* 
ESD Final Project
Creator: Raza Qazi and Ji Zhao
Date: 25 April 2017
Description: This file can receive data from Raspberry Pi seriol port and save data to files.
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>			//Achieve real time
#include <wiringPi.h>		//Wiring library for the Raspberry Pi
#include <wiringSerial.h>	//Raspberry pi serial port library

#define day 1				//first day of a month
#define month "May"			//month
#define year 17				//year
#define f_number 3			//file number
unsigned char data_receive;	//data received
unsigned char fd;			//fd: check serial connection

void setup(void);			//check wiringPi initial
int get_decimal(unsigned char data_receive);//converte data to decimal

void setup(void)
{
    if(-1 == wiringPiSetup())//if wiringPi initial false
    {
	    printf("wiringPi set up error\n\r");
        exit(-1);			 //end code
    }
}


int get_decimal(unsigned char data_receive)
{
    unsigned int data_temp = 0;
    
	while(data_receive != 'Z' )//if receive Z, one data finished transfer
	{
		data_temp = data_temp * 10 + (data_receive - '0');//convert ascii to decimal
		data_receive = serialGetchar(fd);	//continue receive data from serial port
	}
    return data_temp;
}

int main()
{
    FILE  *ofp[f_number];	//output file
    char *mode_w = "w";		//Write data
    char *mode_a = "a";		//Append data
    char *outputFile[f_number];//output file
    /* Real Time */
    //time_t rawtime;
    //struct tm * timeinfo;
    //char timeE[100];

    unsigned char i, j, m_count = 0, t_count = 0;	//m_count: number of moisture data; t_count: number of temperature data
    unsigned char m_data[100];	//moisture data save in this array
    unsigned char t_data[100];	//temperature data save in this array
    int t_data_total = 0;		//using for calculate sum of temperature data
    float t_data_avg = 0;		//using for calculate average of temperature data
    float  t_energy_consume = 0, t_purchase = 0;//t_energy_consume: more energy consumed on temperature; t_purchase: need to pay more
    int  k = 1;					//help jump loop to finish receiving data

    outputFile[0] ="moisture.csv";		//write moisture value to this file
    outputFile[1] = "temperature.csv";	//write temperature value to this file
    outputFile[2] = "consume.csv";		//write monthly consume to this file
    
    setup(); 
    
    if((fd = serialOpen("/dev/ttyS0",9600))==-1)//serial initial, baudrate = 9600, if error setup
    {
	    printf("Serial port set up error\n\r");	
        exit(-1);			//end code
    }
    else
    {
		serialFlush(fd);	//serial initial succeed
	}

    for(i = 0; i < f_number; i++)
    {
        ofp[i] = fopen(outputFile[i], mode_a);	//check if can open a file
        if (ofp[i] == NULL)	//cannot open file
        {
          fprintf(stderr, "Can't open output file %s!\n", outputFile[i]);
          exit(1);			//end code
        }
        else
        {
            if(i == 0)
				fprintf(ofp[i], "date,moisture\r");		//write to moisture file
			else if(i == 1)
				fprintf(ofp[i], "date,temperature\r");	//write to temperature file
			else
				fprintf(ofp[i], "Consumption\r");		//write to consume file
        }
    }
    
    while(k)	//k = 1
    {
        while(serialDataAvail(fd) < 1)			//wait until serial port has data
        {
			printf("Wait for command %d\n\r",k);
			delay(500);
			k++;
		}
        while(serialDataAvail(fd) >= 1)			//if serial buffer has data
        {
            data_receive = serialGetchar(fd);   //return the data(1byte) read from serial
            
            if(data_receive == 'M')     		//if receive characrer M: moisture sensor
            {
                data_receive = serialGetchar(fd); 
                if(data_receive == 'S')     	//if receive characrer S: transfer data to RPi
                {
					j = 1;
					while(j)
					{
						data_receive = serialGetchar(fd);
						
						if(data_receive == 'E')	//if receive characrer E: one sensor data finish all transfer
						{
							j = 0;				//jump moisture sensor loop
						}
						else
						{
							//strftime(timeE, 80, "%d-%b-%y",timeinfo); //Real Time
							m_data[m_count] = get_decimal(data_receive);//convert data to decimal
							m_count++;
						}
					}
				}
            }
            if(data_receive == 'T')				//temperature sensor flag
            {
                data_receive = serialGetchar(fd); 
                if(data_receive == 'S')			//if receive character S,start transfer data to RPi
                {
					j = 1;
					while(j)
					{
						data_receive = serialGetchar(fd);
						if(data_receive == 'E')	//if receive characrer E: one sensor data finish all transfer
						{
							j = 0;				//jump temperature sensor loop
							k = 0;				//jump all loops
						}
						else
						{
							//strftime(timeE, 80, "%d-%b-%y",timeinfo);	//Real Time
							t_data[t_count] = get_decimal(data_receive);//convert data to decimal
							t_count++;
						}	
					}
				}
            }			
        }        
    }
    /* Simulate one month, but in our code, we can use real time*/ 
    if(m_count > 30)
    {
		m_count = 30;
	}
    if(t_count > 30)
    {
		t_count = 30;
	}
	/* Write value to moisture file */
	for(i = 0; i < m_count; i++)
	{
		fprintf(ofp[0], "%d-%s-%d,%d\r", (day+i), month, year, m_data[i]);    //Append to m_outputFile
	}
	/* Write value to temperature file */
	for(i = 0; i < t_count; i++)
	{
		fprintf(ofp[1], "%d-%s-%d,%d\r", (day+i), month, year, t_data[i]);    //Append to t_outputFile
	}
	/* Calculate monthly energy consume and bill */
	for(i = 0; i < t_count; i++)
	{
		t_data_total = t_data_total + t_data[i];
		
	}		
	t_data_avg = t_data_total / t_count;
	t_energy_consume = (t_data_avg - 25) * 7.5 * 30;//threshold is 25, one month has 30 days, 7.5Kw/day
	t_purchase = t_energy_consume * 0.134;			//at a cost of $0.134 per kwh;
	fprintf(ofp[2], "Energy/Monthly: %dkWh, Price/Monthly: $%d\r", (int)(t_energy_consume), (int)(t_purchase));
	
	for(i = 0; i < f_number; i++)
	{
		ofp[i] = fopen(outputFile[i], mode_w);   	//Empty all files
		fclose(ofp[i]);
	}
	
	return 0;
}

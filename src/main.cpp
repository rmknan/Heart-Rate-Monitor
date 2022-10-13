////////// EMBEDDED CHALLENGE PROJECT SPRING 2022 - Submitted by
//  Ashwin Subramnaian- as16012
//  Drishti Singh - ds6730
// Mohanakrishnan - mr5910

//Header Files
#include "mbed.h"   
#include "stdio.h"

void Pressure();
void Systolic_Pressure();
void Diastolic_Pressure();
void HeartRate();

//creating I2C Object //SDA SCL PB9 PB8
I2C PS(PC_9, PA_8); 

Timer t; //Creating a Timer Object

//All pressure calculation variables
float P_min = 0.0; 
float P_max = 300.0;
float P_Slope[1000] = {1.0}; 
float pressure = 0.0; 
bool p_inc = true, p_dec=false; 
int Loop_counter = 0; 
float P_array[1000]; 
float Prev_P =0.0; 
float max_slope=0; 
int index_slope = 0;
int syst_p_index = 0; 
int diast_p_index = 0;
float Hr_no = 0; 

//Deflation rate variables
char fast_def_rate[80] = "Your Deflation is too fast, slow down!"; 
char slow_def_rate[80] = "Your Deflation is too slow, speed up!";
char perf_def_rate[80] = "Optimal Def!ation rate";
char *def_remark; //deflation rate remarks based on change in pressure variable

//All Sensor related variables
int sensor_add = 0x30; //Address of the sensor for I2C communication for writing
int sensor_add_read = 0x31; //Address of the sensor for I2C communication for reading
const char sensor_Command[] = { 0xAA, 0x00, 0x00 }; //Command sent to sensor for reading data
char sensor_Result[4] = {0}; 
float sensor_Output = 0.0; 
float sensor_Output_Min = 419430.4; 
float sensor_Output_Max = 3774873.6; 
char sensor_Result_Status = '0'; 

float t_arr[1000]; //creating a time array

// 536876680
// pf9 pf8 pf7 //pa6 pa7 pa5

void Pressure()
{
    //Flags to check if the pressure increase and decrease
    while(1)
    {
    //     DigitalOut motor(LED1); //the motor starts
    //     DigitalOut valve(LED2); //the valve is closed
    //     led1.write(1); // using the Led pins PG13 to power the motor
    //     led2.write(0); // the valve is closed

        if(pressure > 150) // Check if pressure goes > 150, turn p_inc = false
            p_inc = false;
            //led1.write(0) // set the motor off

        if (!p_inc && pressure < 151) // check if is p_inc is false && and pressure goes < 151, turn p_dec==true
            p_dec = true;
            //led1.write(0) // the motor is off
            //led2.write(1) // open the valve

        if (pressure < 30 and p_dec) // check if pressure goes < 30 and p_dec = true
            break;
            

        PS.write(sensor_add, sensor_Command, 3); //Write the commands 0xAA, 0x00 and 0x00 at the given sensor address
        wait_us(6000);

        PS.read(sensor_add_read, sensor_Result, 4); //Reading the 4 bytes from the sensor and storing in sensor_Result array
        wait_us(6000);
       
        sensor_Output = (float)((sensor_Result[1] << 16) | (sensor_Result[2] << 8) | (sensor_Result[3])); //creating the 24-bit output from the sensor 
     
        pressure = (((sensor_Output - sensor_Output_Min) * (P_max - P_min)) / (sensor_Output_Max - sensor_Output_Min)) + P_min; //calculating the pressure as per the formula in datasheet
     
        // Automating the motor to pump to the device
        // if(pressure <= 150){
        //     DigitalOut led1(LED1);
        //     DigitalOut led2(LED2);
        //     led1.write(1);
        // }
        // else{
        //     DigitalOut led1(LED1);
        //     led1.write(0);
        // }
        
        if((Prev_P-pressure) < 0.8) //check if deflation rate is below 0.8
        {
            def_remark = slow_def_rate; // print "Your Deflation is too slow, speed up!"
        }
        else if ((Prev_P-pressure) > 3.0) //check if deflation rate is above 3.0
        {
            def_remark = fast_def_rate; // print "Your Deflation is too fast, slow down!"
        }
        else 
        {
            def_remark= perf_def_rate; // print "Optimal Deflation rate!"
        }
        int time_ms = t.read_ms(); //read the time value using object "t"

        if(p_dec) //check if the p_dec==true implying that the user has started releasing the pressure
        {
            printf (" Time : %d | Pressure Reading = %.2f  | change %.2f | %s\n", time_ms/1000, pressure,pressure-Prev_P,def_remark); 
            P_array[Loop_counter] = pressure; //store the current calculated pressure into the pressure array
            t_arr[Loop_counter] = time_ms/1000; //store the time in seconds in the time array
            Loop_counter++; //increase the counter value by 1
            // led2.write(1)
        }
        else if(p_inc == false) //if pressure is now pumped beyond 151 
        {
             printf (" Time: %d | Pressure Reading = %.2f | change %.2f | %s \n", time_ms/1000, pressure,pressure-Prev_P, def_remark );
        }
        else
        {   //to print pressure values when pressure is increasing and is less that 151
             printf (" Time: %d | Pressure Reading = %.2f\n", time_ms/1000, pressure);
        }

        //store the current pressure value into Prev_P before calculating the new pressure value
        Prev_P = pressure; 
        wait_us(500000); //sample at every 0.5 seconds
    }

    t.stop(); //stop the timer when pressure calculations get over

    int loop_p1 = 0; //create loop variable
    int loop_p2 = 0; //create loop variable

    //loop for calculating slopes
    for(loop_p1=1;loop_p1<Loop_counter;loop_p1++) 
    {
        float pressure_Difference=P_array[loop_p1]-P_array[loop_p1-1]; //store consecutive pressure differences
        float time_Difference=(t_arr[loop_p1]-t_arr[loop_p1-1]); //store consecutive time differences

        if(time_Difference != 0.000000 ) //flag to prevent getting into an infinite slope
        P_Slope[loop_p1-1]= (pressure_Difference/time_Difference); //store calculated slope values in slope array
    }
    
    for(loop_p2=0; loop_p2<Loop_counter; loop_p2++) //To find the maximum positive slope for MAP
    {

        if(P_Slope[loop_p2] > max_slope)//check if current slope in the array greater than the current max positive slope
        {
            max_slope = P_Slope[loop_p2];// then store the new greater value in max_PositiveSloep
            index_slope = loop_p2+1; //store the index of this slope value
        }
     }

     printf("----------------------------------------------------------------------\n\n");
     printf("---------------------CALCULATED RESULTS!!-----------------------------\n");
     printf("----------------------------------------------------------------------\n\n");
     printf("\nMEAN ARTERIAL PRESSURE: is %.2f\n", P_array[index_slope]);
       
}

void Systolic_Pressure()
{
    float sysSlopeMinThreshold =  0.5 * max_slope; //calculate Systolic pressure slope minimum threshold
    float diffInSlope=0; 
    float minDiffInSlope=INT32_MAX; //stores the smallest difference in slope readings
   
    int loop_s = 0; //loop variable

    //check if any value is lesser than the index of the slope of the MAP
    for(loop_s=0; loop_s < index_slope-1; loop_s++) 
     {
        //check first if slope is positive and less than the systolic threshold value
         if((P_Slope[loop_s]>=0.0) && (P_Slope[loop_s] < sysSlopeMinThreshold))
         {
            //calculate the difference between threshold value and the current slope reading
            diffInSlope = sysSlopeMinThreshold - P_Slope[loop_s]; 
            if(diffInSlope < minDiffInSlope) //check if the calculated difference is less than minimum difference in slope
            {
                minDiffInSlope=diffInSlope; //then store minimum difference in slope as this calculated difference
                syst_p_index=loop_s+1;  //store the corresponding index for the value in pressure array
            }  
         }
     }
    printf("\nSYSTLOIC PRESSURE PATIENT: %.2f \n",  P_array[syst_p_index]);  
}

void Diastolic_Pressure()
{ 
    float diaSlopeMinThreshold =  0.8 * max_slope; //calculate Diastolic pressure slope minimum threshold
    float diffInSlope_dia=0; 
    float minDiffInSlope_dia=INT32_MAX; //stores the smallest difference in slope readings

    int loop_d = 0; //loop variable

    //check if any value is lesser than the index of the slope of the MAP
    for(loop_d=index_slope+1; loop_d < Loop_counter; loop_d++)
     {  
        //check first if slope is positive and is less than the diastolic threshold value
        if((P_Slope[loop_d]>=0.0) && (P_Slope[loop_d] < diaSlopeMinThreshold))
        {
            //calculate the difference between the threshold value and the current slope reading
            diffInSlope_dia = diaSlopeMinThreshold - P_Slope[loop_d];
            if(diffInSlope_dia < minDiffInSlope_dia) //check if the calculated difference is less than minimum difference in slope
            {
                minDiffInSlope_dia=diffInSlope_dia; //then store minimum difference in slope as this calculated difference
                diast_p_index=loop_d+1; //store the corresponding index for the value in pressure array 
            }
        }
     }
    printf("\nDIASTOLIC PRESSURE OF PATIENT: %.2f \n",  P_array[diast_p_index]); 
}

void HeartRate()
{
    
    int loop_hr=0; 
    for(loop_hr = syst_p_index-1; loop_hr < diast_p_index; loop_hr++) //start the pressure slope loop
    {
        if(P_Slope[loop_hr] > 0.0)//check for all positive slopes greater than 0.0
            {
                Hr_no++; //count the number of positive slopes
            }    
    }

   int heart_Rate = (int)(((Hr_no) / (t_arr[diast_p_index] - t_arr[syst_p_index]))* 60.0f);

    //calculate the heart rate by dividing by the time and then multiplying by 60
    printf("\nHEART RATE OF PATIENT: %d beats per minute\n",heart_Rate);
    printf("\n----------------FIN--------------------------- *\n");   
}

int main()
{
    
    t.start(); //timer start condition
    wait_us(500000); //wait for 500ms
    printf("---------------BP MONITORING SYSTEM----------------\n");
    wait_us(600000); //wait for 600 ms
    printf("\n\nStart pumping the cuff to begin the process!\n\n");
    wait_us(500000); //wait for 500ms
  
    Pressure();
    Systolic_Pressure(); //call the systolic pressure calculation function 
    Diastolic_Pressure(); //call the diastolic pressure calculation function 
    HeartRate(); //call the heart rate calculation function 
}
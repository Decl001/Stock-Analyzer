/*
Author: Declan Atkins
Last Modified: 18/08/17

This module is for all operations relating
to the prediction model. This includes writing 
a days value to the model and updating each of
the neccessary files, as well as returning the 
expected value of a stock price.
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

//external functions
bool add_values(double *changes_list, char* company, char* set);
double *get_predicted_values(char *company, char* set);
double make_single_prediction_EXTERNAL(char *company, char* set);
//internal functions
bool append_to_values(double *changes_list, char *company, char* set);
bool update_probabilities(char *company, char* set);
double make_single_prediction_INTERNAL(double last_change,double last_val,char* company, char* set);
double **get_probability_values(char *filename,double prev_val);




/*
This function is called from the python side in order
to add the actual values into the model. It returns true
on a successful completion and false if an error occurs
*/
bool add_values(double *changes_list, char *company, char* set){
	
	bool success;
	
	success = append_to_values(changes_list,company,set);
	if(!success){
		return false;
	}
	else{
		success = update_probabilities(company,set);
	}
	
	return success;
}

/*
This function returns a prediction for the value of 
the stock price at each minute within the market hours
of the following day
*/
double *get_predicted_values(char* company, char* set){
	
	int i;
	double *ret = (double *) malloc(390 * sizeof(double));
	
	for(i=0;i<390;i++){
		
		if(i == 0){
			ret[i] = make_single_prediction_EXTERNAL(company,set);
		}
		else if(i == 1){
			char fname[100] = strcat("../../",strcat(company,"/PREVIOUS_VALUES.dat"))
			FILE* last_values = fopen(fname);
	
			char line[10];
			while(fgets(line,10,last_values){
				last_val = atoi(line);
			}
			
			ret[i] = make_single_prediction_INTERNAL(ret[i-1] - last_val,ret[i-1],company,set);
		}
		else{
			ret[i] = make_single_prediction_INTERNAL(ret[i-1] - ret[i-2],ret[i-1],company,set);
		}
	}
	
	return ret;
}

/*
This function takes in a company name and keyword set. It opens
the file containing the previous values and retrieves the latest
value, then calls the make_single_prediction_INTERNAL function to 
return a prediction based on that value
*/
double make_single_prediction_EXTERNAL(char* company, char* set){
	
	
	char fname[100] = strcat("../../",strcat(company,"/PREVIOUS_VALUES.dat"))
	FILE* last_values = fopen(fname);
	char fname1[100] = strcat("../../",strcat(company,"/PREVIOUS_CHANGES.dat"))
	FILE* last_changes = fopen(fname1);
	double last_change;
	
	char line[10];
	while(fgets(line,10,last_values){
		last_val = atoi(line);
	}
	
	char line1[10];
	while(fgets(line1,10,last_changes){
		last_change = atoi(line1);
	}
	
	return make_single_prediction_INTERNAL(last_change,last_val,company,set);
}

/*
This function is used to make a single prediction based on the
last value (either actual or predicted) and return it. It uses
the weighting values that are dynamically updated when new data
is added to the model in order to weight the relevance of each 
of the last 100 values to the next value. It also takes in set
of keywords that are dominant at this time period and uses the
relevant model based on these values.
*/
double make_single_prediction_INTERNAL(double last_change,double last_val,char* company, char* set){
	
	int N_FILES = 100;
	FILE *WEIGHTING;
	
	int i;
	char *line;
	
	int weighting_values[N_FILES];
	
	
	//read in the dynamicaly assigned weighting values
	char fname[100] = strcat("../../",strcat(company,"/WEIGHTING.dat"));
	WEIGHTING = fopen(fname,"r");
	
	for(i=0;i<100;i++){
		
		char line[3];
		fgets(line,3,WEIGHTING);
		weighting_values[i] = atoi(line);
		
	}
	
	fclose(WEIGHTING);
	
	double list_expected_changes[100];
	
	for(int i=1;i<=100;i++){
		char i_str[3];
		sprintf(i_str,"%d",i);
		char filename[100] = strcat(strcat("../../", strcat(company,"/")),strcat(i_str, ".dat\0"));
		double ** change_probality_list = get_probability_values(filename,last_val);
		
		double expected_change = 0;
		for(;*change_probality_list;change_probality_list++){
			expected_change += (*change_probality_list[0]) * (*change_probality_list[1]);  
		}
		
		list_expected_changes[i] = expected_change;
	}
	
	double actual_expected_change=0;
	for(i=0;i<100;i++){
		actual_expected_change += weighting_values[i]*list_expected_changes[i];
	}
	
	return last_val + actual_expected_change;
}


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
#include <string.h>
#include <stdbool.h>

struct model_data{//Struct for the model linked list
	double change;
	double expected_change;
	int count;
	struct model_data *next;
}model_data;

struct change_node{//struct for the linked list of changes
	double change;
	struct change_node *next;
	struct change_node *prev;
}change_node;

//external functions
void add_values(double *values_list,int n_values, char* company, char* set);
double *get_predicted_values(char *company, char* set);
double make_single_prediction_EXTERNAL(char *company, char* set);

//internal functions
double append_to_values(double *values_list,int n_values, char *company);
void update_probabilities(char *company, char* set,double last_val);
double make_single_prediction_INTERNAL(struct change_node *HEAD,double last_val,char* company, char* set);
double get_expected_value(char *filename,double prev_val);
void update_weighting_values(double *values_list,int n_values,char *company, char *set);
char *read_last_line(char *filename); 

/*
This function is called from the python side in order
to add the actual values into the model. It returns true
on a successful completion and false if an error occurs
*/
void add_values(double *values_list,int n_values, char *company, char* set){
	
	double last_val;
	update_weighting_values(values_list,n_values,company,set);
	last_val = append_to_values(values_list,n_values,company,set);
	update_probabilities(company,set,last_val);
	
	return;
}

/*
This function returns a prediction for the value of 
the stock price at each minute within the market hours
of the following day
*/
double *get_predicted_values(char* company, char* set){
	
	int i;
	double *ret = (double *) malloc(390 * sizeof(double));
	
	char filename_vals[50];
	sprintf(filename_vals, "../Data/%s/PREVIOUS_VALUES.dat", company);
	double last_val = atof(read_last_line(filename_vals));
	char fname1[100];
	strcpy(fname1,"../Data/");
	strcat(strcat(fname1,company),"/PREVIOUS_CHANGES.dat");
	FILE* last_changes = fopen(fname1, "r");
	struct change_node *HEAD = NULL;
	struct change_node *TAIL = NULL;
	struct change_node *CURR;

	double last_change;
	char line1[10];
	int n_changes = 0;
	while(fgets(line1,10,last_changes)){
		last_change = atof(line1);
		if(n_changes == 0){
			TAIL = HEAD = (struct change_node*) malloc(sizeof(struct change_node));
		}
		CURR = (struct change_node*) malloc(sizeof(struct change_node));
		CURR->change = last_change;
		CURR->next = HEAD;
		HEAD->prev = CURR;
		HEAD = CURR;
		n_changes++;
	}

	while(n_changes < 100){//append 0s to end if not enough changes, likely won't ever be needed
		CURR = (struct change_node*) malloc(sizeof(struct change_node));
		CURR->change = 0.0;
		CURR->prev = TAIL;
		TAIL->next = CURR;
		TAIL = CURR;
		n_changes++;
	}

	for(i=0;i<390;i++){
		ret[i] = make_single_prediction_INTERNAL(HEAD,last_val,company,set);
		//use TAIL here to recycle memory and delink vals that are no longer needed
		TAIL->change = ret[i] - last_val;
		TAIL->prev = NULL;
		HEAD->prev = TAIL;
		TAIL->next = HEAD;
		HEAD = TAIL;
		last_val = ret[i];
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
	
	struct change_node *HEAD = NULL;
	struct change_node *TAIL = NULL;
	struct change_node *CURR = HEAD;
	char fname[100];
	strcpy(fname,"../Data/");
	strcat(strcat(fname,company),"/PREVIOUS_VALUES.dat");
	FILE* last_values = fopen(fname, "r");
	char fname1[100];
	strcpy(fname1,"../Data/");
	strcat(strcat(fname1,company),"/PREVIOUS_CHANGES.dat");
	FILE* last_changes = fopen(fname1, "r");
	double last_val;
	char line[10];
	while(fgets(line,10,last_values)){
		last_val = atof(line);
	}
	double last_change;
	char line1[10];
	int n_changes = 0;
	while(fgets(line1,10,last_changes)){
		last_change = atof(line1);
		CURR = (struct change_node*) malloc(sizeof(struct change_node));
		CURR->change = last_change;
		CURR->next = HEAD;
		HEAD->prev = CURR;
		HEAD = CURR;
		if(n_changes == 0){
			TAIL = HEAD;
		}
		n_changes++;
	}

	while(n_changes < 100){//append 0s to end if not enough changes, likely won't ever be needed
		CURR = (struct change_node*) malloc(sizeof(struct change_node));
		CURR->change = 0.0;
		CURR->prev = TAIL;
		TAIL->next = CURR;
		TAIL = CURR;
		n_changes++;
	}
	
	return make_single_prediction_INTERNAL(HEAD,last_val,company,set);
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
double make_single_prediction_INTERNAL(struct change_node *HEAD,double last_val,char* company, char* set){
	
	FILE *WEIGHTING;
	struct change_node * CURR = HEAD;
	int i;
	
	double weighting_values[100];
	
	
	//read in the dynamicaly assigned weighting values
	char fname[100];
	strcpy(fname,"../Data/");
	strcat(strcat(strcat(strcat(strcat(fname,company),"/"),set),"/"),"WEIGHTING.dat");
	WEIGHTING = fopen(fname,"r");
	
	for(i=0;i<100;i++){
		
		char line[10];
		fgets(line,3,WEIGHTING);
		weighting_values[i] = atof(line);
		
	}
	fclose(WEIGHTING);
	
	double list_expected_changes[100];
	
	for(int i=0;i<100;i++){
		char i_str[3];
		sprintf(i_str,"%d",i);
		char filename[100];
		strcpy(filename,"../Data/");
		strcat(strcat(strcat(strcat(strcat(strcat(filename,company),"/"),set),"/"),i_str),".dat");

		double last_change = CURR->change;
		CURR = CURR->next;

		double expected_change = get_expected_value(filename,last_change);
		
		list_expected_changes[i] = expected_change;
	}
	
	double actual_expected_change=0;
	for(i=0;i<100;i++){
		actual_expected_change += weighting_values[i]*list_expected_changes[i];
	}
	//printf("AEC = %.3lf\n", actual_expected_change);
	
	return last_val + actual_expected_change;
}


/*
This function opens the file passed to it and then searches in
it for the value that it takes in. It then returns the expected 
change given the value
*/
double get_expected_value(char* filename, double prev_val){
	
	double expected_value = prev_val;
	FILE *fp = fopen(filename,"r");
	
	double buff_change,buff_expected;
	int buff_count;
	while(fscanf(fp,"%lf expected:%lf count:%d", &buff_change,&buff_expected,&buff_count) == 3){
		
		if (prev_val == buff_change){
			expected_value = buff_expected;
		}
	}
	
	fclose(fp);
	return expected_value;
	
}

/*
This function updates the model with the new data that has just
been recieved.
*/
void update_probabilities(char *company, char *set,double last_val){
	
	char filename[100];
	strcpy(filename,"../Data/");
	strcat(strcat(filename,company),"/PREVIOUS_VALUES.dat");
	double changes_list[390];//Maximum number of values 
	FILE *fp = fopen(filename,"r");
	double next_val;
	int len_changes=0;//number of actual changes 
	char line[15];
	while(fgets(line,13,fp)){
		sscanf(line,"%lf",&next_val);
		changes_list[len_changes++] = next_val - last_val;
		last_val = next_val;
		
	}
	fclose(fp);
	int i;
	
	int x;
	for(x=0;x<100;x++){
		
		char x_str[3];
		sprintf(x_str,"%d",x);
		struct model_data *HEAD = NULL;
		struct model_data *CURR = HEAD;
		int len_model = 0;
		char filename1[100];
		strcpy(filename1,"../Data/");
		strcat(strcat(strcat(strcat(strcat(strcat(filename1,company),"/"),set),"/"),x_str),".dat");
		double buff_change, buff_expected;
		int buff_count;
		
		FILE *fp1 = fopen(filename1, "r");
		if(fp1 == NULL){
			fp1 = fopen(filename1, "w");
			fclose(fp1);
			fp1 = fopen(filename1, "r");
		}
		char line1[50];
		while(fgets(line1,48,fp1)){
			sscanf(line1,"%lf expected:%lf count:%d",&buff_change,&buff_expected,&buff_count);
			CURR = malloc (sizeof(struct model_data));
			CURR->next = HEAD;
			CURR->change = buff_change;
			CURR->count = buff_count;
			CURR->expected_change = buff_expected;
			HEAD = CURR;
		}
		fclose(fp1);
		int i;
		for(i=0;i<len_changes-x-1;i++){
			bool found = false;
			for(CURR = HEAD;CURR != NULL;CURR = CURR->next){
				
				if(changes_list[i] - CURR->change > -0.0001 && changes_list[i] - CURR->change < 0.0001){
					CURR->expected_change = ((CURR->count/(CURR->count + 1.0)) * CURR->expected_change) + (1.0/(CURR->count+1)) * changes_list[i+x+1];
					CURR->count++;
					found = true;
					break;
				}
			}

			if (!found){
				struct model_data * NEW = malloc (sizeof(struct model_data));
				NEW->next = HEAD;
				NEW->change = changes_list[i];
				NEW->count = 1;
				NEW->expected_change = changes_list[i+x+1];
				HEAD = NEW;
			}
		}

	
		FILE *fp2 = fopen(filename1,"w");
	
		for(CURR = HEAD;CURR != NULL;CURR = CURR->next){
		
			fprintf(fp2,"%.3lf expected:%.3lf count:%d\n", CURR->change,CURR->expected_change,CURR->count);
		}
	
		fclose(fp2);
	}

	char change_filename[50];
	sprintf(change_filename,"../Data/%s/PREVIOUS_CHANGES.dat", company);
	FILE *changes_fp;
	changes_fp = fopen(change_filename, "w");

	for(i=0;i<len_changes;i++){
		fprintf(changes_fp,"%.3lf\n",changes_list[i]);
	}
	fclose(changes_fp);
	return;
}

/*
This function takes in the list and writes it to the list of
previous values in the file for the company given
*/
double append_to_values(double *values_list,int n_values, char *company){
	
	double last_val;
	
	FILE *fp;
	char filename[100];
	
	strcpy(filename,"../Data/");
	strcat(strcat(filename,company),"/PREVIOUS_VALUES.dat");
	char buffer_line[20];
	char *line = malloc(20);
	
	char* excess;
	line = read_last_line(filename);
	sscanf(line,"%lf",&last_val);
	fp = fopen(filename,"w");
	int i;
	for(i=0;i<n_values;i++){
		fprintf(fp,"%.3lf\n",values_list[i]);
	}
	fclose(fp);
	return last_val;
	
}

/*
TO BE IMPLEMENTATED LATER
*/
void update_weighting_values(double *values_list,int n_values, char *company, char *set){
	
    return;
}

/*
This function takes in a filename, opens the file and return the last
line of that file in char* format
*/
char *read_last_line(char *filename){
    FILE *fp;
    char buff[21];

    if(fp = fopen(filename, "rb")){
        fseek(fp,-21,SEEK_END);
        fread(buff, 20, 1, fp);            
        fclose(fp);                               

        buff[20] = '\0';                   
        char *last_newline = strrchr(buff, '\n'); 
        char *last_line = last_newline+1;         

        return last_line;
    }
    else{
        return "0.0";
    }
}
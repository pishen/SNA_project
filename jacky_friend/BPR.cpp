#include "parse.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <fstream>

#define K 10 // number of features
#define SIGMA 1e-5 // noise for random initialization
#define RATE 0.01 // Learning Rate
#define LAMBDA 0.0 // Regularization Weight

enum EntityType{USER,PHOTO,POST};

vector<string>* users = new vector<string>();
vector<string>* photos = new vector<string>();
vector<string>* posts = new vector<string>();

map<string,double*>* user_feature = new map<string,double*>();
map<string,double*>* photo_feature = new map<string,double*>();
map<string,double*>* post_feature = new map<string,double*>();

vector< pair<string,string> >* photo_user = new vector< pair<string,string> >();
vector< pair<string,string> >* post_user = new vector< pair<string,string> >();

vector< pair<string,string> >* photo_user_train = new vector< pair<string,string> >();
vector< pair<string,string> >* photo_user_test = new vector< pair<string,string> >();
vector< pair<string,string> >* post_user_train = new vector< pair<string,string> >();

vector< pair<string,string> >* post_user_test = new vector< pair<string,string> >();

/////////////////// Printer
void printArr(double* arr,int size){
	
	cerr << "[" ;
	for(int i=0;i<size;i++)
		cerr << arr[i] << ",";
	cerr << "]";
}

void printMap(map<int,double*>* m){

	map<int,double*>::iterator iter;
	cerr << "[" ;
	for(iter=m->begin();iter!=m->end();iter++){
		cerr << iter->first << "=>" ;
		printArr(iter->second,K);
		cerr << ",";
	}
	cerr << "]" ;
}

void printSet(set<int>* s){
	
	set<int>::iterator iter;
	cerr << "(" ;
	for(iter = s->begin(); iter != s->end() ; iter++){
		
		cerr << *iter << "," ;
	}
	cerr << ")" ;
}

void printPair(pair<int,int> pair){
	
	cerr << pair.first << "_" << pair.second ;
}
///////////////////End Printer


///////////////////Vector Operation
double dot(double* a, double* b){
	double sum=0.0;
	for(int i=0;i<K;i++)
		sum += a[i]*b[i];
	return sum;
}

double* vmul(double c, double* v1){
	
	double* v2 = new double[K];
	for(int i=0;i<K;i++)
		v2[i] = v1[i]*c ;
	return v2;
}

double* vadd(double* v1, double* v2){
	
	double* v3 = new double[K];
	for(int i=0;i<K;i++)
		v3[i] = v1[i] + v2[i] ;
	return v3;
}

double* vsub(double* v1, double* v2){
	
	double* v3 = new double[K];
	for(int i=0;i<K;i++)
		v3[i] = v1[i] - v2[i] ;
	return v3;
}
//////////////////END Vector Operation

Parser* parser;
void readData(char* fname){
	
	parser = new Parser(fname);

	//construt user list
	users->push_back(parser->host);
	(*user_feature)[parser->host] = new double[K];
	for(map<string,string>::iterator it=parser->friend_names.begin(); 
	    it!=parser->friend_names.end(); it++){
		
		users->push_back(it->first);
		(*user_feature)[it->first] = new double[K];
	}

	//construct photo list, photo_user list
	for(map<string,Photo*>::iterator it=parser->photos.begin(); it!=parser->photos.end(); it++){
		photos->push_back(it->first);
		(*photo_feature)[it->first] = new double[K];
		
		vector<string>* u_vect = &( it->second->people );
		for(int i=0;i<u_vect->size();i++){

		    if( u_vect->at(i)=="" )
			    continue;

		    photo_user->push_back( make_pair( it->first, u_vect->at(i) ) );
		    if(  user_feature->find( u_vect->at(i) )==user_feature->end() ){
			    user_feature->insert( make_pair(u_vect->at(i),new double[K]) );
			    users->push_back(u_vect->at(i));
		    }
		}
	}

	//construct post list, post_user list
	for(map<string,Message*>::iterator it=parser->messages.begin(); it!=parser->messages.end(); it++){
		posts->push_back(it->first);
		(*post_feature)[it->first] = new double[K];
		
		post_user->push_back( make_pair(it->first,parser->host) );

		vector<string>* u_vect = &(it->second->who_likes);
		for(int i=0;i<u_vect->size();i++){
		    
		    if( u_vect->at(i)=="" )
			    continue;
		    
		    post_user->push_back( make_pair(it->first,u_vect->at(i)) );

		    if( user_feature->find( u_vect->at(i) )==user_feature->end() ){
			    user_feature->insert( make_pair(u_vect->at(i),new double[K]) );
			    users->push_back(u_vect->at(i));
		    }
		}

		u_vect = &(it->second->who_comments);
		for(int i=0;i<u_vect->size();i++){
		    
		    if( u_vect->at(i)=="" )
			    continue;
		    
		    post_user->push_back( make_pair(it->first,u_vect->at(i)) );
		    
		    if( user_feature->find( u_vect->at(i) )==user_feature->end() ){
			    user_feature->insert( make_pair(u_vect->at(i),new double[K]) );
			    users->push_back(u_vect->at(i));
		    }
		}
	}
}

void train_test_partition(int a,int b){ //partition as a : b
	
	for(int i=0;i<photo_user->size();i++)
		if(rand()%(a+b)<a)
			photo_user_train->push_back(photo_user->at(i));
		else
			photo_user_test->push_back(photo_user->at(i));
	
	for(int i=0;i<post_user->size();i++)
		if(rand()%(a+b)<a)
			post_user_train->push_back(post_user->at(i));
		else
			post_user_test->push_back(post_user->at(i));
}

void init_features(){
	
	srand(time(NULL));
	map<string,double*>::iterator iter;
	for(iter=user_feature->begin();iter!=user_feature->end();iter++){
		for(int k=0;k<K;k++)
			(iter->second)[k] = 0 - (SIGMA/2) + ((double)rand()/RAND_MAX)*SIGMA ;
	}
	for(iter=photo_feature->begin();iter!=photo_feature->end();iter++){
		for(int k=0;k<K;k++)
			(iter->second)[k] = 0 - (SIGMA/2) + ((double)rand()/RAND_MAX)*SIGMA ;
	}
	for(iter=post_feature->begin();iter!=post_feature->end();iter++){
		for(int k=0;k<K;k++)
			(iter->second)[k] = 0 - (SIGMA/2) + ((double)rand()/RAND_MAX)*SIGMA ;
	}
}

vector<string>* getList(EntityType type){
	
	switch(type){
		case USER: return users;
		case PHOTO: return photos;
		case POST: return posts;
	}
}

map<string,double*>* getFeatureList(EntityType type){

	switch(type){
		case USER: return user_feature;
		case PHOTO: return photo_feature;
		case POST: return post_feature;
		default: cout << "no such type !!" << endl; exit(0);
	}
}

string draw_neg_example(EntityType type){ //type can be USER,POST,PHOTO
	
	vector<string>* list = getList(type) ;
	
	return list->at( rand() % (list->size()) ) ;
}

double compute_score(pair<string,string> pair, EntityType type1, EntityType type2){
	
	string e1 = pair.first; 
	string e2 = pair.second; 
	double *fea1,*fea2;

	map<string,double*>* m = getFeatureList(type1);
	map<string,double*>::iterator it = m->find( e1 );
	if(it==m->end())
		cout << e1 << " not found in feature list." << endl;
	else
		fea1 = it->second;
	
	m = getFeatureList(type2);
	it = m->find(e2);
	if(it==m->end())
		cout << e2 << " not found in feature list." << endl;
	else 
		fea2 = it->second;

	return dot(fea1,fea2); 
}

void train_features(int iter){ // no garbage collection for now

	vector< pair<string,string> >* pairList;
	pair<string,string>* pair;
	string neg1,neg2 ;//negative example
	double *fea1, *fea2, *fea_neg1, *fea_neg2;//features of user,tag,item
	double *grad_fea1, *grad_fea2, *grad_fea_neg1, *grad_fea_neg2 ;//greadients of features
	double err_grad1, err_grad2, score_pos, score_neg1, score_neg2, prob1, prob2 ;

	int a = photo_user_train->size();
	int b = post_user_train->size();
	for(int i=0;i<iter;i++){
		
		//draw a sample from photo_user/post_user
		int ran = rand()%(a+b);
		EntityType type1 = (ran<a)?PHOTO:POST;
		EntityType type2 = USER;
		pairList = (ran<a)?photo_user_train:post_user_train;
		pair = &( pairList->at( rand() % pairList->size() ) );
		
		//get features of 1st and 2nd
		fea1 = getFeatureList(type1)->find(pair->first)->second;
		fea2 = getFeatureList(type2)->find(pair->second)->second; //second must be a user
		
		//draw negative example for 1st and 2nd
		neg1 = draw_neg_example(type1);
		neg2 = draw_neg_example(type2);
		fea_neg1 = getFeatureList(type1)->find(neg1)->second ;
		fea_neg2 = getFeatureList(type2)->find(neg2)->second ;
		
		//get features of 1st and 2nd
		score_pos = compute_score( *pair,type1,type2);
		score_neg1 = compute_score( make_pair(neg1,pair->second) ,type1,type2 );
		score_neg2 = compute_score( make_pair(pair->first,neg2) ,type1,type2 );
	
			
		prob1 = 1.0/( 1 + exp(-(score_pos-score_neg1)) ); //prob( pair->first > other )
		prob2 = 1.0/( 1 + exp(-(score_pos-score_neg2)) ); //prob( pair->second > other )
		err_grad1 = 1-prob1;
		err_grad2 = 1-prob2;
		
		//in view of 2nd > other
		grad_fea1 = vsub( vmul(err_grad2, vsub(fea2,fea_neg2) ) , vmul(LAMBDA,fea1) ) ;
		grad_fea2 = vsub( vmul(err_grad2,fea1) , vmul(LAMBDA,fea2) );
		grad_fea_neg2 = vsub( vmul(-err_grad2,fea1) , vmul(LAMBDA,fea_neg2) );
	
		//in view of 1st > other
		
		grad_fea2 = vadd( grad_fea2 , vsub( vmul(err_grad1, vsub(fea1,fea_neg1) ) , vmul(LAMBDA,fea2) ) );
		grad_fea1 = vadd( grad_fea1 , vsub( vmul(err_grad1,fea2) , vmul(LAMBDA,fea1) ) );
		grad_fea_neg1 = vsub( vmul(-err_grad1,fea2) , vmul(LAMBDA,fea_neg1) );
		
		
		(*getFeatureList(type1))[pair->first] = vadd( fea1 , vmul(RATE,grad_fea1) );
		(*getFeatureList(type2))[pair->second] = vadd( fea2 , vmul(RATE,grad_fea2) );
		(*getFeatureList(type1))[neg1] = vadd( fea_neg1 , vmul(RATE,grad_fea_neg1) );
		(*getFeatureList(type2))[neg2] = vadd( fea_neg2 , vmul(RATE,grad_fea_neg2) );

	}
}

//test can be 0:first or 1:second
double sample_rank_err(vector< pair<string,string> >* pairList_test,EntityType type1,EntityType type2,int test){
	
	pair<string,string>* pair;
	string neg ; //negative tag
	double *fea1, *fea2, *fea_neg;//features of user,tag,item
	double score_pos, score_neg,  prob , lnErr;
	
	double sum =0.0;
	for(int i=0;i<pairList_test->size();i++){
		
		pair = &(pairList_test->at(i));
		fea1 = getFeatureList(type1)->find(pair->first)->second;
		fea2 = getFeatureList(type2)->find(pair->second)->second;
		
		//draw negative tag example (consider draw negatie item example in the future)
		EntityType type_neg = (test==0)?type1:type2;
		neg = draw_neg_example(type_neg) ;
		fea_neg = getFeatureList(type_neg)->find(neg)->second ;
		
		score_pos = compute_score(*pair,type1,type2);
		if(test==0)
			score_neg = compute_score(make_pair(neg,pair->second),type1,type2);
		else
			score_neg = compute_score(make_pair(pair->first,neg),type1,type2);
		
		prob = 1.0/( 1 + exp(-(score_pos-score_neg)) );
		lnErr = log( 1-prob );
		sum += lnErr;
	}

	return sum/pairList_test->size() ;
}

void dump_user_feature(){
	
	ofstream fout1("users.txt");
	ofstream fout2("user_features.txt");
	for(int i=0;i<users->size();i++){
		string user = users->at(i);
		string name = parser->friend_names[user];
		double* fea = user_feature->find(user)->second ;
		
		fout1 << user << "\t" << ((name=="")?"NULL":name)  << endl ;

		for(int k=0;k<K;k++)
			fout2 << fea[k] << "\t" ;
		fout2 << endl;
	}
	fout1.close();
	fout2.close();
}

void dump_photo_feature(){
	
	ofstream fout1("photos.txt");
	ofstream fout2("photo_features.txt");
	for(int i=0;i<photos->size();i++){
		string photo = photos->at(i);
		double* fea = photo_feature->find(photo)->second ;
		
		fout1 << photo << endl ;

		for(int k=0;k<K;k++)
			fout2 << fea[k] << "\t" ;
		fout2 << endl;
	}
	fout1.close();
	fout2.close();
}

int main(int argc, char** argv){
	
	readData(argv[1]);
	
	train_test_partition(8,2);

	init_features();

	for(int i=0;i<=70;i++){
		train_features(100000);
		cout << "photo_err=" << sample_rank_err(photo_user,PHOTO,USER,1) << "\t";
		cout << "post_err=" << sample_rank_err(post_user,POST,USER,1) << "\n";
		//cout << "test_err=" << sample_rank_err(photo_user_test,PHOTO,USER,1) << endl;
	}
	dump_user_feature();
	dump_photo_feature();
	
	return 0;
}

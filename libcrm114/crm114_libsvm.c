// LIBSVM classifier.  Copyright 2001-2010 William S. Yerazunis.
//
//   This file is part of the CRM114 Library.
//
// The CRM114 Library is free software: you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
//   The CRM114 Library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with the CRM114 Library.  If not, see <http://www.gnu.org/licenses/>.

//  This file is derived (er, "cargo-culted") from crm114_markov.c, so
//  some of the code may look really, really familiar.
//  
//  This file is dual licensed to Bill Yerazunis and Huseyin Oktay; both have
//  full rights to the code in any way desired, including the right to 
//  relicense the code in any way desired.
//
//
//   This version uses multiple class blocks, rather than unifying
//   the classes together into one block.

//carbon copy from hyperspace-nn

//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

#include "crm114_lib.h"



#include "crm114_internal.h"




//This is copied from libsvm package
//Just to make the malloc statements as short as possible
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

//This is a global variable because different functions is using this data structure.

//struct crm114_feature_row *allFeatures; 
//This has the total number of unique features in the allFeatures data structure
//long par;

//used in classify function
//stores the features in terms of their id's for the text that is to be classified.

struct svm_node *x;
   
//Required for LIBSVM
      	
    
	
//Holds the learned model structure. The model comes back from the LIBSVM library
struct svm_model *model;
	
//holds the feature values for the problem. This is used in training phase. 
struct svm_node *x_space;
	
//Holds the feature space  This is used in classification phase.
struct svm_node *feature_space;

//This saves the model structure into a file
//modelFilename: is the actual path of the file that the model structure will be saved in.
//If the string is empty, it saves into a default directory.

CRM114_ERR crm114_save_model_libsvm(char *modelFilename)

{
  char model_file_name[1024];
  char temp[1024];
	
  sprintf( temp, "%d", rand());
	
  if(strcmp(modelFilename, "")==0)
    {
      strcpy(model_file_name,"/homes/oktay/models/models_09_29/model");
      strcat(model_file_name,temp);
      strcat(model_file_name,".out");
    }
  else
    strcpy(model_file_name,modelFilename);
		
  svm_save_model(model_file_name,model);
	
  return CRM114_OK;
}


//  This function stores the features in the training set into hashes
//   document by document.

CRM114_ERR crm114_store_features_libsvm (CRM114_DATABLOCK **db,
					 int whichclass,
					 const struct crm114_feature_row features[],
					 long featurecount)
{
  long i;
  
  // hash table (the class we're to learn into) and its length.
  //This is pretty much the same structure that is used in crm114_hyperspace.c
  
  HYPERSPACE_FEATUREBUCKET *hashes;
  int microgroom;
  int nextslot; 


  if (crm114__internal_trace)
    fprintf (stderr,
	     "starting learn, class %d, blocksize: %zu, used: %zu adding: %zu"
	     " bytes of features\n",
	     whichclass,
	     (*db)->cb.block[whichclass].allocated_size,
	     (*db)->cb.block[whichclass].size_used,
	     featurecount * sizeof (HYPERSPACE_FEATUREBUCKET));

  if (db == NULL || features == NULL)
    return CRM114_BADARG;

  //class labels should start from 0, otherwise it is a BAD ARGUMENT

  if (whichclass < 0 || whichclass > (*db)->cb.how_many_classes - 1)
    return CRM114_BADARG;

  if ((*db)->cb.classifier_flags & CRM114_REFUTE)
    return CRM114_BADARG;

  //  GROT GROT GROT  we don't microgroom yet.
  microgroom = 0;
  if ((*db)->cb.classifier_flags & CRM114_MICROGROOM)
    {
      microgroom = 1;
      if (crm114__user_trace)
	fprintf (stderr, " enabling microgrooming.\n");
    };

  if ((*db)->cb.classifier_flags & CRM114_ERASE)
    {
      if (crm114__user_trace)
	fprintf (stderr,
		 "Sorry, ERASEing is not yet supported in hyperspace!\n");
      return (CRM114_BADARG);
    };

  //These comments below are valid for libsvm as well. That's why they are not removed.
	
  //   Hyperspace "learning" (ignoring erasing) is really, really simple.
  //   Just append the features (without the feature-row numbers) to
  //   the class.  Use a 0 for a sentinel.
  //
  //   Note that crm114_learn_features() knows to always sort the incoming
  //   features, so we can just append them here without having to
  //   sort them again (and thus, within a document, the two-finger
  //   comparison will work).
  //
  //   Each of the following quantities is treated as a 32-bit unsigned int
  //   (including the zero sentinels, that mark the end of each doc):
  //
  //      feature feature  feature  feature... zero
  //
  //   GROT GROT GROT ???  For future implementation (not needed yet):
  //
  //     Erasing: finding a near-identical series and splicing it out.
  //     Do that later, not now.
  //

  //  Make sure we have enough space (features + sentinel)

  if ((*db)->cb.class[whichclass].features
      + (*db)->cb.class[whichclass].documents
      + featurecount + 1 >
      (*db)->cb.block[whichclass].allocated_size / sizeof (HYPERSPACE_FEATUREBUCKET))
    {
      size_t new_block_size;
      int errstate;
      if (crm114__user_trace)
	fprintf (stderr, "Sorry, insufficient space in db.  Must expand\n");
      //return (CRM114_CLASS_FULL);
      //
      //    Make this block big enough...
      //    We use a "double up" algorithm - that is, whenever a block
      //    is too small, we double it plus the size of the new information
      //    required to be stored.  So, on the average, a block is
      //    75% full, which is fine by us.

      new_block_size = 2 * (*db)->cb.block[whichclass].allocated_size
	+ featurecount * (int) sizeof (HYPERSPACE_FEATUREBUCKET)  ;

      errstate = crm114__resize_a_block (db, whichclass, new_block_size);
      if (errstate != CRM114_OK) return (errstate);

	
    };

  hashes =
    (HYPERSPACE_FEATUREBUCKET *)
    (&((*db)->data[(*db)->cb.block[whichclass].start_offset]) );


  //     Append our features to the hash & update feature count
  for (i = 0; i < featurecount; i++)
    {
      //  Nextslot is zero-based, which is why this next thing is NOT ...+1
      nextslot =
	(*db)->cb.class[whichclass].documents
	+ (*db)->cb.class[whichclass].features;
      hashes[nextslot].hash = features[i].feature;
      //  reserve the zero hash value as a sentinel; promote zeroes to +1
      if (hashes[nextslot].hash == 0)
	hashes[nextslot].hash = 1;
      (*db)->cb.class[whichclass].features ++;

    };


  //  put in the zero sentinel at the end and update learns count
  nextslot =
    (*db)->cb.class[whichclass].documents
    + (*db)->cb.class[whichclass].features;
  hashes[nextslot].hash = 0;
  (*db)->cb.class[whichclass].documents++;

  //   Update estimate of remaining space
  (*db)->cb.block[whichclass].size_used =
    ( (*db)->cb.class[whichclass].documents
      + (*db)->cb.class[whichclass].features)
    * (int)sizeof (HYPERSPACE_FEATUREBUCKET);


  if (crm114__internal_trace)
    {
      fprintf (stderr, "finished storing example data \n");
      for (i = 0; i < (*db)->cb.how_many_classes; i++)
	fprintf (stderr, " ...class %ld, start %zu alloc %zu used %zu, learns: %d feats: %d\n",
		 i,
		 (*db)->cb.block[i].start_offset,
		 (*db)->cb.block[i].allocated_size,
		 (*db)->cb.block[i].size_used,
		 (*db)->cb.class[i].documents,
		 (*db)->cb.class[i].features);
      fprintf (stderr, " ... returning.\n");
    }

  //   All done, return OK.

  return (CRM114_OK);
}
//get the size of the model structure
//required to allocate enough space for the block in the db that stores the model structure
size_t calcuateSizeofModel(struct svm_model *model)
{
	
  //This is based on the standard model structure
  //model->svm_type --->int
  //model->kernel_type --->int
  //model->nr_class --->int
  //model->l --->int
  //model->rho --->double (we have n(n-1)*n values for this where n is the number of classes)
  //model->label --->int (we have n values for this where n is the number of classes)
  //model->probA --->double (we have n(n-1)*n values for this where n is the number of classes)
  //model->probB --->double (we have n(n-1)*n values for this where n is the number of classes)
  //model->nSV --->int (we have n values for this where n is the number of classes)
  //model-->svcoef -->double (we have (n-1)*model-->l values for this where n is the number of classess,
  // and model-->l is the total number of support vectors)
  //and we have varying number of features where for each feature we have an integer index value and double value.
  size_t size=0;
	
  size = (sizeof(int)*2*model->nr_class+4)+(sizeof(double)*3*(model->nr_class*(model->nr_class-1)/2))
    + (sizeof(double)*((model->nr_class-1)*model->l));
	
  int count=0;
	
  for(int i=0;i<model->l;i++)
    {
      const struct svm_node *p = model->SV[i];
		
      if(model->param.kernel_type == PRECOMPUTED)
	{
	  size += sizeof(int)+sizeof(double);	
	}
      else
	{
	  while(p->index != -1)
	    {					
	      count++;
				
	      p++;
	    }
	}//else	
    }//for
  size += (sizeof(int)+sizeof(double))*count;

  return size;
}

void setDefaultParameters(struct svm_parameter* param)
{
  //see the readme file in the libsvm folder for further details on these parameters
  // default values
  //classification or regression
  param->svm_type = C_SVC;
	
  param->kernel_type = LINEAR;
  param->degree = 3;
  param->gamma = 0.0001220703125;	// 1/num_features
  param->coef0 = 0;
  param->nu = 0.5;
  param->cache_size = 100;
  param->C = 32;
  param->eps = 1e-3;
  param->p = 0.1;
  param->shrinking = 1;
  //calculate probabibility estimates-->(1) or not -->(0)
  param->probability = 1;//set this to 1 because crm114 needs probability estimates
  param->nr_weight = 0;
  param->weight_label = NULL;
  param->weight = NULL;
  //cross_validation = 0;

}


int readInt(char *readPtr, size_t offset)
{
  int tempBuffer=0;
	
  size_t datasize=sizeof(int);

  memcpy((char *)&tempBuffer,readPtr+offset, datasize);
		
  return tempBuffer;
}

size_t writeInt(char *writePtr, size_t offset, int value)
{
  size_t datasize=sizeof(int);

  memcpy(writePtr+offset,(char *)&value, datasize);
		
  return datasize;
			
}
//This function saves the model structure into the last block in the db

CRM114_ERR  writeModelIntoBlock(CRM114_DATABLOCK **db){
	
  char *writeCharPtr;
  
  int whichblock = (*db)->cb.how_many_blocks-1; //index of the second to the last block
   
  size_t BUFSIZE = calcuateSizeofModel(model)+ 1000000;//Have extra 8mb just in case
   
       
  //Resize the block
  if(BUFSIZE > (*db)->cb.block[whichblock].allocated_size)
    crm114__resize_a_block (db,whichblock,
			    BUFSIZE);
  else
    BUFSIZE = (*db)->cb.block[whichblock].allocated_size;
			 
  writeCharPtr = (char *)(&(*db)->data[0] + (*db)->cb.block[whichblock].start_offset);
  size_t datasize=0, nextpos=0, freepos=0;
		
  if (nextpos > BUFSIZE)
    {	
      //      printf("Out of memory\n");
      //      exit(0);
      return (CRM114_NOMEM);
    }

  datasize=sizeof(int);
  nextpos=freepos+datasize;
	
  //sprintf( temp, "%d", model->param.svm_type );
  int tempBuffer = model->param.svm_type;
	
  memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);
		
  freepos=nextpos;

  ///////////////////////////////////////
	
  if (nextpos > BUFSIZE)
    {	
      // printf("Out of memory\n");
      // exit(0);
      return (CRM114_NOMEM);
    }

  datasize=sizeof(int);
  nextpos=freepos+datasize;
  tempBuffer = model->param.kernel_type;
  memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);
  freepos=nextpos;
	
  ///////////////////////////////////////

  if (nextpos > BUFSIZE)

    {
      // printf("Out of memory\n");
      //  exit(0);
      return (CRM114_NOMEM);
    }

  datasize=sizeof(int);
  nextpos = freepos + datasize;

  tempBuffer = model->nr_class;
	
  memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);

  freepos=nextpos;

  ///////////////////////////////////////

  if (nextpos > BUFSIZE)

    {
      //    printf("Out of memory\n");
      // exit(0);
      return (CRM114_NOMEM);
    }
  datasize=sizeof(int);
  nextpos = freepos + datasize;
  tempBuffer = model->l;
  memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);
  freepos=nextpos;
	
  ///////////////////////////////////////
		
  if (nextpos > BUFSIZE)

    {
      //     printf("Out of memory\n");
      // exit(0);
      return (CRM114_NOMEM);
    }
  int howmanyrho = model->nr_class*(model->nr_class-1)/2;
  datasize=sizeof(double);
  double tempDouble;	
  for(int i=0;i<howmanyrho;i++)
    {
      nextpos = freepos + datasize;
      tempDouble = model->rho[i];
      memcpy(writeCharPtr+freepos,(char *)&tempDouble, datasize);
      freepos=nextpos;
    }

  ///////////////////////////////////////
	
  if (nextpos > BUFSIZE)
    {
      //      printf("Out of memory\n");
      //exit(0);
      return (CRM114_NOMEM);
    }
	
  datasize=sizeof(int);
	
  for(int i=0;i<model->nr_class;i++)
    {
      nextpos = freepos + datasize;
      tempBuffer = model->label[i];
      memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);
      freepos=nextpos;
    }
	
  ///////////////////////////////////////
	
  if (nextpos > BUFSIZE)
    {
      //      printf("Out of memory\n");
      //exit(0);
      return (CRM114_NOMEM);
    }
  int howmanyprobA = model->nr_class*(model->nr_class-1)/2;
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobA;i++)
    {
      nextpos = freepos + datasize;
      tempDouble = model->probA[i];
      memcpy(writeCharPtr+freepos,(char *)&tempDouble, datasize);
      freepos=nextpos;
    }
	
  ///////////////////////////////////////    
	
  if (nextpos > BUFSIZE)
    {
      //     printf("Out of memory\n");
      //exit(0);
      return (CRM114_NOMEM);
    }
  int howmanyprobB = model->nr_class*(model->nr_class-1)/2;
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobB;i++)
    {
      nextpos = freepos + datasize;
      tempDouble = model->probB[i];
      memcpy(writeCharPtr+freepos,(char *)&tempDouble, datasize);
      freepos=nextpos;
    }

  ///////////////////////////////////////  
	
  if (nextpos > BUFSIZE)
    {
      //      printf("Out of memory\n");
      //exit(0);
      return (CRM114_NOMEM);
    }
  datasize=sizeof(int);
  for(int i=0;i<model->nr_class;i++)
    {
      nextpos = freepos + datasize;
      tempBuffer = model->nSV[i];
      memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);
      freepos=nextpos;
    }

  /////////////////////////////////////// 
		
  for(int i=0;i<model->l;i++)
    {
      for(int j=0;j<model->nr_class-1;j++)
	{
	  datasize=sizeof(double);
	  nextpos = freepos + datasize;
	  tempDouble = model->sv_coef[j][i];
	  memcpy(writeCharPtr+freepos,(char *)&tempDouble, datasize);
	  freepos=nextpos;
	}

      const struct svm_node *p = model->SV[i];
		
      if(model->param.kernel_type == PRECOMPUTED)
	{
	  datasize=sizeof(double);
	  nextpos = freepos + datasize;
	  tempDouble = (double)(p->value);
	  memcpy(writeCharPtr+freepos,(char *)&tempDouble, datasize);
	  freepos=nextpos;
		
	}
      else
	{
	  while(p->index != -1)
	    {
	      datasize=sizeof(int);
	      nextpos = freepos + datasize;
	      tempBuffer = p->index;
	      memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);
	      freepos=nextpos;
	      datasize=sizeof(double);
	      nextpos = freepos + datasize;
	      tempDouble = p->value;
	      memcpy(writeCharPtr+freepos,(char *)&tempDouble, datasize);
	      freepos=nextpos;
	      p++;
	    }
			
	  datasize=sizeof(int);
	  nextpos = freepos + datasize;
	  tempBuffer = -1;
	
	  memcpy(writeCharPtr+freepos,(char *)&tempBuffer, datasize);
	  freepos=nextpos;
			
			
	}//else	
			
		
    }//for
    
    
  //Rethink about this
  (*db)->cb.block[whichblock].size_used = freepos;

  return CRM114_OK;
}

//this function classifies a given set of features and puts the result values into the results structure.

int readFromBlock(CRM114_DATABLOCK *db, struct svm_model *modelStructure)
{
		
  char *readPtr;
  int whichblock = (db)->cb.how_many_blocks-1;
  readPtr = (char *)(&(db)->data[0] + (db)->cb.block[whichblock].start_offset);	
  size_t datasize=0, nextpos=0, freepos=0;

  ///////////////////////////////////////
	
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  int temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  modelStructure->param.svm_type = temp2Buffer;
  freepos=nextpos;

  ///////////////////////////////////////
	
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  modelStructure->param.kernel_type = temp2Buffer;
  freepos=nextpos;
	
  ///////////////////////////////////////

  datasize=sizeof(int);
  nextpos = freepos + datasize;
  temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  modelStructure->nr_class = temp2Buffer;
  freepos=nextpos;

  ///////////////////////////////////////
	
  datasize=sizeof(int);
  nextpos = freepos + datasize;
  temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  modelStructure->l = temp2Buffer;
  freepos=nextpos;
	
  ///////////////////////////////////////
	
  int howmanyrho = modelStructure->nr_class*(modelStructure->nr_class-1)/2;
  modelStructure->rho = Malloc(double,howmanyrho);
  datasize=sizeof(double);
  double tempDouble;
  for(int i=0;i<howmanyrho;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
      modelStructure->rho[i]=tempDouble;
      freepos=nextpos;
    }
	
  //////////////////////////////////////
	
  modelStructure->label = Malloc(int,modelStructure->nr_class);
  datasize=sizeof(int);	
  for(int i=0;i<modelStructure->nr_class;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
      modelStructure->label[i]=temp2Buffer;
      freepos=nextpos;
    }
	
  //////////////////////////////////////
		
  int howmanyprobA = modelStructure->nr_class*(modelStructure->nr_class-1)/2;
  modelStructure->probA = Malloc(double,howmanyprobA);
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobA;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
      modelStructure->probA[i]=tempDouble;
      freepos=nextpos;
    }
	
  //////////////////////////////////////
	
  int howmanyprobB = modelStructure->nr_class*(modelStructure->nr_class-1)/2;
  modelStructure->probB = Malloc(double,howmanyprobB);
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobB;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
      modelStructure->probB[i]=tempDouble;
      freepos=nextpos;
    }
	
  //////////////////////////////////////
	
  modelStructure->nSV = Malloc(int,modelStructure->nr_class);
  datasize=sizeof(int);
  for(int i=0;i<modelStructure->nr_class;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
      modelStructure->nSV[i]=temp2Buffer;
      freepos=nextpos;
    }
	
  //////////////////////////////////////
  //Allocate memory
	
  int m = modelStructure->nr_class - 1;
  int l = modelStructure->l;
  modelStructure->sv_coef = Malloc(double *,m);
  for(int i=0;i<m;i++)
    modelStructure->sv_coef[i] = Malloc(double,l);
  modelStructure->SV = Malloc(struct svm_node *,modelStructure->l);
	
  //get the length of the total number of SV
  //we need this to allocate the right amount of space
  int numofFeatures=0;
  size_t nextposTemp=nextpos, freeposTemp=freepos;
	
  for(int i=0;i<modelStructure->l;i++)
    {
      for(int j=0;j<modelStructure->nr_class-1;j++)
	{
	  datasize=sizeof(double);
	  nextposTemp = freeposTemp + datasize;
	  memcpy((char *)&tempDouble,readPtr+freeposTemp, datasize);
	  //modelStructure->sv_coef[j][i]=tempDouble;
	  freeposTemp=nextposTemp;
	}
		
      if(modelStructure->param.kernel_type == PRECOMPUTED)
	{
	  datasize=sizeof(double);
			
	  nextposTemp = freeposTemp + datasize;
	  memcpy((char *)&tempDouble,readPtr+freeposTemp, datasize);
	  numofFeatures++;
	  freeposTemp=nextposTemp;
			
	}
      else
	while(1)
	  {
	    datasize=sizeof(int);
				
	    nextposTemp = freeposTemp + datasize;
	    memcpy((char *)&temp2Buffer,readPtr+freeposTemp, datasize);
	    freeposTemp=nextposTemp;
				
	    if(temp2Buffer == -1)
	      break;//end of feature sett
				
	    datasize=sizeof(double);
							
	    nextposTemp = freeposTemp + datasize;
	    memcpy((char *)&tempDouble,readPtr+freeposTemp, datasize);
	    freeposTemp=nextposTemp;
	    numofFeatures++;			
	  }
    }
	
  numofFeatures += modelStructure->l;
	
  //feature_space = Malloc(struct svm_node, numofFeatures);
  feature_space = (struct svm_node *)malloc(numofFeatures*sizeof(struct svm_node));
	
  int featureInd=0;
	
  ///To get the number of Features in the SV's
	
	
  datasize=sizeof(double);
	
  for(int i=0;i<modelStructure->l;i++)
    {
      for(int j=0;j<modelStructure->nr_class-1;j++)
	{
	  datasize=sizeof(double);
	  nextpos = freepos + datasize;
	  memcpy((char *)&tempDouble,readPtr+freepos, datasize);
	  modelStructure->sv_coef[j][i]=tempDouble;
	  freepos=nextpos;
	}
      //giving error in this line
      //need to figure out the dimensions of SV
      //Remember SV is a two dimensional structure.
      //one dimension is modelStructre.l
      //but I have to figure out the other dimension.

      if(modelStructure->param.kernel_type == PRECOMPUTED)
	{
	  datasize=sizeof(double);
			
	  nextpos = freepos + datasize;
	  memcpy((char *)&tempDouble,readPtr+freepos, datasize);
	  feature_space->value = tempDouble;
	  freepos=nextpos;
			
	}
      else
	{
	  modelStructure->SV[i] = &feature_space[featureInd];
			
	  while(1)
	    {
	      datasize=sizeof(int);
				
	      nextpos = freepos + datasize;
	      memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
	      feature_space[featureInd].index=temp2Buffer;
	      freepos=nextpos;
				
	      if(temp2Buffer == -1)
		{
		  featureInd++;
		  break;//end of feature sett
		}
				
	      datasize=sizeof(double);
							
	      nextpos = freepos + datasize;
	      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
	      feature_space[featureInd].value=tempDouble;
	      freepos=nextpos;
				
	      featureInd++;
	    }//while
	}//else
		
    }
	
	
  return 0;
}

int binarySearch (struct crm114_feature_row *allFeatures, int size,unsigned int value)
{
  int ti=0;
  int lowBound=0;
  int highBound=size;
	
	
  for(int revLimit=0;revLimit<255;revLimit++)
    //while(1)
    {
      ti=(lowBound + highBound)/2;
			
      if(value == allFeatures[ti].feature)
	return ti;
      if(value == allFeatures[ti+1].feature)
	return ti+1;
      if(value <allFeatures[ti].feature)
	highBound = ti;
      else
	lowBound =ti;	
    }
  // printf("Failure to find the feature! \n");
  return 0;
		
}


//  this function stores the features, then actually solves the problem
CRM114_ERR crm114_learn_features_libsvm (CRM114_DATABLOCK **db,
					 int class,
					 const struct crm114_feature_row features[],
					 long featurecount)
{
  int whichclass;
  whichclass = class;
  struct crm114_feature_row *fr;
  fr = features;
  long nfr;
  nfr = featurecount;
  int err;

  err = CRM114_OK;

  //  First, if we have any features to store, store them!
  if (nfr > 0)
    err=crm114_store_features_libsvm (db, whichclass, features, featurecount);

  if (err != CRM114_OK || ((*db)->cb.classifier_flags & CRM114_APPEND))
    return (err);

  //  Second, if we don't have at least one document in each class, we should
  //   return with a BADARG, as Lin's LIBSVM code fails miserably with
  //    no data input!
  int iclass; 
  for (iclass = 0; iclass < (*db)->cb.how_many_classes; iclass++)
    if ((*db)->cb.class[iclass].documents < 1)
      return (CRM114_BADARG);

  //  This function solves and finds the linear model by using the libsvm library
  //  Here first we densify the feature, and then remove the dupcliates from the features.
  //  Every unique feature gets an id number.
  int elements,j;
		     	
  // printf("Solving the optimization problem by using the LIBSVM Library.\n");

  HYPERSPACE_FEATUREBUCKET *ht; 
  HYPERSPACE_FEATUREBUCKET *ht_scan;  
  int htlen=0;   
  int upt=0, kpt=0;
  int documentNumber=0; //Number of documents in the training set.
  	
  int docIndex=0;
  	
  //  Set by parse_command_line stores the parameters needed for the svm solver. 
  //   For more details see the pdf titled as "A practical Guide for SVM 
  //    classification" http://www.csie.ntu.edu.tw/~cjlin/papers/guide/guide.pdf
  struct svm_parameter param;		
    
  //Holds the problem instance (i.e., training sets and features)
  struct svm_problem prob;		// set by read_problem
  	

  //   How many features are there, total;
  int htTotalLen = 0;

  //total number of features in each class
  int classFeatureSize[DEFAULT_HOW_MANY_CLASSES];
  int featureIndex=0;
	
  int sort=1;
  int unique=1;
  int classInd=0;
  int *densePtr;
  
  long idx;  //holds the feature id for the svm file
  int val;   //value of the feature id, most of the time 1 in our case. 
   			
  for (classInd = 0; classInd < (*db)->cb.how_many_classes; classInd++)
    {
      htTotalLen += (*db)->cb.class[classInd].documents 
	+ (*db)->cb.class[classInd].features; 
      classFeatureSize[classInd] = (*db)->cb.class[classInd].documents 
	+ (*db)->cb.class[classInd].features;
    }

  //  Allocating enough space for the feature row vector that stores all 
  //   the features in the training corpus.  
  //  allFeatures is the densification table
  
  struct crm114_feature_row *allFeatures; 

  //  This has the total number of unique features in the allFeatures data structure
  long par;
  
  if ((allFeatures = malloc(htTotalLen * sizeof(struct crm114_feature_row))) == NULL)
    {  return (CRM114_NOMEM);    };
    
  //   Initially ht_scan is the start of block 0
  ht_scan = (HYPERSPACE_FEATUREBUCKET *) (&(*db)->data[0] + (*db)->cb.block[0].start_offset);

  featureIndex =0;

  for (classInd = 0; classInd < (*db)->cb.how_many_classes; classInd++)
    {		
      htlen = (*db)->cb.class[classInd].documents + (*db)->cb.class[classInd].features;  
    	
      //   get a pointer to the known features hash table
      ht_scan = (HYPERSPACE_FEATUREBUCKET *) 
	(&(*db)->data[0] +  (*db)->cb.block[classInd].start_offset);
      upt=0;
      unsigned valueStore;
      while (upt < htlen)
	{
	  //get the hash values and put everything into allFeatures		
	  valueStore = 	ht_scan[upt].hash;
	  allFeatures[featureIndex].feature=valueStore;
	  upt++; 
	  featureIndex++;
	}       // end of while
    } //  end of for loop
	
  //  total number of features in the training corpus
  par = (long)featureIndex-1;
  //  Now we need to remove the duplicates from the (feature densification table) and 
  //  sort the features according to the hash values
  //  Here note that par is an in/out variable.
  //   first you give the total number of features (duplicates are counted twice)
  //    then you get out the unique number of features.

  crm114_feature_sort_unique(allFeatures, &par, sort, unique);
      
  //   Do a safety scan - did we actually get what we expected?
  for(int i=0;i<par-1;i++)
    {
      if(allFeatures[i].feature == 0)
      	{
	  // printf("howl!! i=%d\n",i);
      	}      	
      if(allFeatures[i].feature>allFeatures[i+1].feature)
	printf("Out of order! at i=%d\n",i);
    }
    
  fprintf (stderr, "Total unique features found: %ld\n", par);

  //save allFeatures (feature densification table) into last block of the db
    
  int whichblock = (*db)->cb.how_many_blocks-2; //index of the second to the last block
  
  //   Make sure there's enough space to hold the SVM densification vector.  
  int newsize;
  newsize = ((size_t) par * sizeof (int) * 2  + 16000);
  if ( newsize < 2000000)newsize = 2000000;

  if (newsize > (*db)->cb.block[whichblock].allocated_size)
    {
      //fprintf (stderr, "Resizing to %d\n", newsize);
      err = crm114__resize_a_block (db, whichblock, newsize) ;
      //fprintf (stderr, "Resizing done to %d\n", newsize);
    }
  if (err != CRM114_OK) return (err);

  densePtr = (int*)(&(*db)->data[0] + (*db)->cb.block[whichblock].start_offset);
  (*db)->cb.block[whichblock].size_used = par * sizeof(int); 
    
  //   Copy the allFeatures sorted uniqued vector into the db
  unsigned valueStore;
  for (j=0;j<par;j++)
    {
      valueStore = allFeatures[j].feature;    // BOOM HERE!
      densePtr[j] = valueStore;
      //  We store the values in feature densification table to the block
      //  because crmm114_libsvm_classify_features needs the densification 
      //  table to figure out the features in the to-be-classified text.
    }
     


  //   Get the required numbers for allocation
  //  documentNumber--> how many documents in the training set?
  //  elements-->number of features in the training set, total number of features.
       
  for (classInd = 0; classInd < (*db)->cb.how_many_classes; classInd++)
    {
      documentNumber += (*db)->cb.class[classInd].documents;
    }
   
  //  setting up the struct svm_prob, which stores the problem instance 
  //  with all traning corpus information. 
  prob.l = documentNumber;
  //total number of features + documents in the training corpus
  elements= (featureIndex-1)+documentNumber;//because for each document we put -1 at the end
  //the true labels of the training corpus
  prob.y = Malloc(double,prob.l);
  //pointers point to the feature set for each document
  prob.x = Malloc(struct svm_node *,prob.l);
  //feature vector, that stores all the features in the training corpus
  x_space = Malloc(struct svm_node,elements );

  if (x_space == NULL)
    {
      //      printf("x_space structure cannot be allocated!\n");
      //exit(0);
      return (CRM114_NOMEM);
    }
	
  j=0;
  docIndex=0; //index starts from 0 for each class
	  	
  for (classInd = 0; classInd < (*db)->cb.how_many_classes; classInd++)
    {
      htlen = (*db)->cb.class[classInd].documents + (*db)->cb.class[classInd].features;  
      ht = (HYPERSPACE_FEATUREBUCKET *) (&(*db)->data[0] + (*db)->cb.block[classInd].start_offset);
      		      					 
      upt = 0;  
      kpt = 0; 
 
      prob.y[docIndex] = (double)classInd+1.0;
      prob.x[docIndex] = &x_space[j];
      docIndex++;
	
      while (kpt < htlen) //oktay: htlen-> number of features + number of documents
	//   kpt: kpt is iterating over known features meaning the 
	//  whole set of the features learnt during the training phase
	{
	  /*if(kpt == 271796)
	    printf("Stop here!");
	    if(j == 1280136)
	    printf("Stop here!");	*/
	  // are we done with this known document? 0 is the sentinel and determines the end of document in the hash set.
	  if (ht[kpt].hash == 0)
	    {
	      x_space[j].index = -1; //  in x_space, we have index=-1 for the end 
	                             //of each document. This is the convention in LIBSVM.
	      j++;	    		
	      kpt++;
		    		  
	      if( kpt >= htlen)
		{
		  ; /// printf("bbb\n");
		}
	      else
		{
		  prob.y[docIndex] = (double)classInd+1.0;
		  prob.x[docIndex] = &x_space[j];
		  docIndex++;
		}//ELSE
		    		
	    }//IF
	  else
	    {
		    		
	      if (ht[kpt].hash != 0){
		if(allFeatures[11537].feature ==0)
		  printf("Now the value is 0\n");
		//printf("Allfeature 11537 = %d\n",allFeatures[11537].feature);
		//idx = binarySearch(allFeatures,par,ht[kpt].hash)+1;
		    			
		    			
		int ti=0;
		int lowBound=0;
		int highBound=par;
		unsigned int value = ht[kpt].hash;
						
						
		for(int revLimit=0;revLimit<255;revLimit++)
		  //while(1)
		  {
		    ti=(lowBound + highBound)/2;
								
		    if(value == allFeatures[ti].feature)
		      {
			idx = ti+1;
			break;
		      }
		    if(value == allFeatures[ti+1].feature)
		      {
			idx = ti+2;
			break;
		      }
		    if(value <allFeatures[ti].feature)
		      highBound = ti;
		    else
		      lowBound =ti;	
								
		  }
							
		//idx = ti+1;
						
		//printf("values is : %u, id is: %ld\n",value,idx);
		    			
		    					
		//idx=ik+1;
		val=1.0;
									
		x_space[j].index =idx;
		x_space[j].value = (double)val;
		j++;
						
		if(prob.x==NULL)
		  // printf("Geldi");
		  return (CRM114_UNK);
	      }
		    		
	      //par stores the unique number of features in the total training corpus
	      //for (ik=0;ik<par;ik++)
	      /*{
		if (ht[kpt].hash != 0){
		//if (ht[kpt].hash == allFeatures[ik].feature)
		{//GROT GROT GROT!
		//do a binary search instead of linear search
									
		idx = binarySearch(allFeatures,par,ht[kpt].hash)+1;		
		//idx=ik+1;
		val=1.0;
									
		x_space[j].index =idx;
		x_space[j].value = (double)val;
																					
		j++;
		//break;								
		}				
		}//IF
		}//FOR	    		    		
	      */		    		
	      kpt++;
	      //   end of two-finger algorithm
	    }//ELSE
	}//end of while	
    }//for over classInd
    
  //this sets the struct svm_param structure that LIBSVM requires
  //svm_param stores the parameter values required for the 

  setDefaultParameters(&param);
   
#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
	  "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
	  end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
	  (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
	  (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif
   
  //This function actually learns the model and returns it 
   
  if(prob.x[0]->index == 0)
    // printf("Geldi");
    return (CRM114_UNK);

  model = svm_train(&prob,&param);
   
  //Instead read it from the file.
  
  /*	strcpy(model_file_name,"/homes/oktay/models/model.out");
   
	if((model=(struct svm_model *)svm_load_model(model_file_name))==0)
	{
	fprintf(stderr,"can't open model file\n");
	exit(1);
	}
  */
   
   
#if TIMECHECK
  times ((void *) &end_time);
  gettimeofday ((void *) &end_val, NULL);
  printf (
	  "Elapsed time: %9.6f total, %6.3f user, %6.3f system.\n",
	  end_val.tv_sec - start_val.tv_sec + (0.000001 * (end_val.tv_usec - start_val.tv_usec)),
	  (end_time.tms_utime - start_time.tms_utime) / (1.000 * sysconf (_SC_CLK_TCK)),
	  (end_time.tms_stime - start_time.tms_stime) / (1.000 * sysconf (_SC_CLK_TCK)));
#endif
   
  //This function below writes the learned model into the last block in the db
  //we do this because we need the model in the crm114_libsvm_classify_features function
   
  writeModelIntoBlock(db);
   
  crm114_save_model_libsvm("");  
   
  //destroy parameter object for LIBSVM
  svm_destroy_param(&param);
  //free the problem, we no longer need that structure
  free(prob.y);
  //we no longer need the feature set in the problem structure. We are storing it in the densification table.
  free(x_space);
  free(prob.x);
	
	
  svm_destroy_model(model);
  //The densification table is saved into a block in the db. So we can free this structure as well.
  free(allFeatures);
	
  return (CRM114_OK);
}

void svm_free_model(struct svm_model* model)
{
  if(model->free_sv && model->l > 0)
    free((void *)(model->SV[0]));
  for(int i=0;i<model->nr_class-1;i++)
    free(model->sv_coef[i]);
  free(model->SV);
  free(model->sv_coef);
  free(model->rho);
  free(model->label);
  free(model->probA);
  free(model->probB);
  free(model->nSV);
  //free(model);
}

CRM114_ERR crm114_classify_features_libsvm( CRM114_DATABLOCK *db,
					    const struct crm114_feature_row features[],
					    long featurecount,
					    CRM114_MATCHRESULT *result)
{

  long idx;//holds the feature id for the svm file
  int val; //value of the feature id, most of the time 1 in our case. I just have it for the sake of completeness.
  long htTotalLen=0;
  int classInd=0;
  int classFeatureSize[DEFAULT_HOW_MANY_CLASSES];	
  long i,j ;
  int *densePtr;
  double predict_label;
	
  struct svm_model *modelStructure;
  struct svm_model md;

  
      
  //This function reads the model back in from the db
  //We need the model because LIBSVM uses the model to classify the given instance.
 
  readFromBlock(db,&md);
  modelStructure = &md;
  
  //modelStructure = &md;
 		


  //if((model=(struct svm_model *)svm_load_model(model_file_name))==0)
  //	{
  //	fprintf(stderr,"can't open model file\n");
  //exit(1);
  //}

  //Then form the X matrix. I may need to go over all the features in the hash to get the correct id values in the feature row I have here
  //feature row set is already passed to this function.
  //for each feature,
  //go through the features in the hash
  //find the correspondence in the hash and get the id number
  //form the x vector with index and values
	
	
	
  for (classInd = 0; classInd < (db)->cb.how_many_classes; classInd++)
    {
      htTotalLen += (db)->cb.class[classInd].documents + (db)->cb.class[classInd].features; 
      classFeatureSize[classInd]= (db)->cb.class[classInd].documents + (db)->cb.class[classInd].features;
    }//
    
  struct crm114_feature_row *allFeatures; 
  //This has the total number of unique features in the allFeatures data structure
  long par;
    
    
  
    
  int whichblock = (db)->cb.how_many_blocks-2;
      
  densePtr = (int*)(&(db)->data[0] + (db)->cb.block[whichblock].start_offset);
    
  par=0;
  par = (db)->cb.block[whichblock].size_used / sizeof(int);
    
  if ((allFeatures = malloc(par * sizeof(struct crm114_feature_row))) == NULL)
    {
      //      printf("Cannot Allocate Allfeatures vector\n");
      //exit(0);
      return (CRM114_NOMEM);
    }
    

  //Read back in the densification table from the block in the db.
    
  for (j=0;j<par;j++)
    {
      allFeatures[j].feature = densePtr[j];
    }
	
  x = (struct svm_node *) malloc((featurecount+1)*sizeof(struct svm_node));
	
  j=0;//iterates over existing features in the hash function
	
  //GROT GROT GROT
  //For now the hits in the results block are the total number features that are in whole densification table (all classes included)
  //We do not have the per class division for hits for now!.
  int hits=0;
	
  for (i = 0; i < featurecount; i++)//this is how you iterate over each feature in the feature row
    {
    	
      int ti=0;
      int lowBound=0;
      int highBound=par;
      unsigned int value = features[i].feature;
      if (features[i].feature != 0)
	{
						
	  for(int revLimit=0;revLimit<255;revLimit++)
	    //while(1)
	    {
	      ti=(lowBound + highBound)/2;
								
	      if(value == allFeatures[ti].feature)
		{
		  idx = ti+1;
		  break;
		}
	      if(value == allFeatures[ti+1].feature)
		{
		  idx = ti+2;
		  break;
		}
	      if(value <allFeatures[ti].feature)
		highBound = ti;
	      else
		lowBound =ti;	
								
	    }
	  val=1.0;
	  x[j].index =idx;
	  x[j].value = (double)val;
	  j++;
	  hits++;
	}	
      /*for (ik=0;ik<par;ik++)
	if (features[i].feature != 0)
	{
	if (features[i].feature == allFeatures[ik].feature)
	{
	//printf("%d:1 ",ik+1);
	idx=ik+1;
	//idx=features[i].feature;
	val=1.0;
								
	x[j].index =idx;
	x[j].value = (double)val;
	j++;
	hits++;
	break;								
	}				
	}
      */   
    }
    
  x[j].index = -1;//This is the sentinel value for the features indicating the end of document.
  //convention for LIBSVM	

  //This stores the probability estimates for each class.

  double *probabilities = Malloc(double, (db)->cb.how_many_classes);

  //  Fill in the preamble part of the results block
  crm114__clear_copy_result (result, &db->cb);
  
  //Get the label from LIBSVM library
  predict_label = svm_predict_probability(modelStructure,x,probabilities);
	
  //if it is not able to fill out the probabilities, only get the label.
  //if probA probB values in the model structure is really small
  //LIBSVM does not calculate the probabilities if probA or probB is really small 
  //due to numerical errors in the method that is used to calculate the probabilities.
  //In other words, when the probA or probB values are really small, the method that calcuates the probability estimates
  //in LIBSVM makes numerical errors. To prevent this case, probability estimates are simply set to 1/Total_number_of_classes
  //For more details see multiclass_probability function in libsvm/libsvm-2.91/svm.cpp
	
  int count=0;
  for(int i=0;i<modelStructure->nr_class;i++)
    {
      if(probabilities[i] == 1.0/modelStructure->nr_class)
	count++;
    }
		
  if(count==modelStructure->nr_class)
    {//This is the situation where LIBSVM is not able to calculate the probability estimates.
			
      predict_label = svm_predict(modelStructure,x);
      //So get only the label.
			
      for(int i=0;i<modelStructure->nr_class;i++)
	{//Set the probability estimates manually.
	  probabilities[i]=0.99/(double)modelStructure->nr_class;
	  if(i==predict_label-1)
	    probabilities[i]+=0.01;//the one with the actual label, gets a tiny bit more probability estimate value
					
	}
			
    }
  //set the feature count int the result block.
  result->unk_features = (int)featurecount;
	
  for (int k = 0; k < db->cb.how_many_classes; k++)
    {
      result->class[k].prob = probabilities[k];
      result->class[k].hits = hits;
      //set the success probability.
      //Note that for LIBSVM classes start from 1
      //However, for CRM114, classes start from 0
      //that's why we shift the predicted label by 1 with subtraction
      if(k==predict_label-1)
	result->class[k].success = 1;
      else
	result->class[k].success = 0;
    }; 
      
  free(allFeatures);
  free(x);
  svm_free_model(modelStructure);//free(modelStructure);
  free(probabilities);
	  
  //Calculate PR outcome
  crm114__result_pR_outcome(result);
  	
  return CRM114_OK;
}


int writeTheModel(const CRM114_DATABLOCK *db,
		  FILE *fp)
{
  char *readPtr;
  int whichblock = (db)->cb.how_many_blocks-1;
  int howManyClass = (db)->cb.how_many_classes;
      
  //reading the model back in
  readPtr = (char *)(&(db)->data[0] + (db)->cb.block[whichblock].start_offset);
  size_t datasize=0, nextpos=0, freepos=0;

  /////////////////////////////////////// svm_type
	
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  int temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  fprintf(fp, "%d\n", temp2Buffer); 
  freepos=nextpos;

  /////////////////////////////////////// kernel_type
	
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  fprintf(fp, "%d\n", temp2Buffer);
  int kernel_type =temp2Buffer;
  freepos=nextpos;
	
  /////////////////////////////////////// how many class

  datasize=sizeof(int);
  nextpos = freepos + datasize;
  temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  fprintf(fp, "%d\n", temp2Buffer);
  freepos=nextpos;

  /////////////////////////////////////// length of Model
	
  datasize=sizeof(int);
  nextpos = freepos + datasize;
  temp2Buffer=10;
  memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
  fprintf(fp, "%d\n", temp2Buffer);
  int lengthOfModel = temp2Buffer;
  freepos=nextpos;
	
  ///////////////////////////////////////
	
  int howmanyrho = howManyClass*(howManyClass-1)/2;
  datasize=sizeof(double);
  double tempDouble;
  for(int i=0;i<howmanyrho;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
      fprintf(fp, "%f\n", tempDouble);
      freepos=nextpos;
    }
	
  //////////////////////////////////////
	
  datasize=sizeof(int);
  for(int i=0;i<howManyClass;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
      fprintf(fp, "%d\n", temp2Buffer);
      freepos=nextpos;
    }
	
  //////////////////////////////////////
		
  int howmanyprobA =howManyClass*(howManyClass-1)/2;
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobA;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
      fprintf(fp, "%f\n", tempDouble);
      freepos=nextpos;
    }
	
  //////////////////////////////////////
	
  int howmanyprobB = howmanyprobA;
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobB;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
      fprintf(fp, "%f\n", tempDouble);
      freepos=nextpos;
    }
	
  //////////////////////////////////////
		
  datasize=sizeof(int);
  for(int i=0;i<howManyClass;i++)
    {
      nextpos = freepos + datasize;
      memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
      fprintf(fp, "%d\n", temp2Buffer);
      freepos=nextpos;
    }
	
  int featureInd=0;
  datasize=sizeof(double);
  for(int i=0;i<lengthOfModel;i++)
    {
      for(int j=0;j<howManyClass-1;j++)
	{
	  datasize=sizeof(double);
	  nextpos = freepos + datasize;
	  memcpy((char *)&tempDouble,readPtr+freepos, datasize);
	  fprintf(fp, "%f\n", tempDouble);
	  freepos=nextpos;
	}

      if(kernel_type == PRECOMPUTED)
	{
	  datasize=sizeof(double);
	  nextpos = freepos + datasize;
	  memcpy((char *)&tempDouble,readPtr+freepos, datasize);
	  fprintf(fp, "%f\n", tempDouble);
	  freepos=nextpos;
	}
      else
	{
	  while(1)
	    {
	      datasize=sizeof(int);
					
	      nextpos = freepos + datasize;
	      memcpy((char *)&temp2Buffer,readPtr+freepos, datasize);
	      fprintf(fp, "%d\n", temp2Buffer);
	      freepos=nextpos;
					
	      if(temp2Buffer == -1)
		{
		  featureInd++;
		  break;//end of feature sett
		}

	      datasize=sizeof(double);
	      nextpos = freepos + datasize;
	      memcpy((char *)&tempDouble,readPtr+freepos, datasize);
	      fprintf(fp, "%f\n", tempDouble);
	      freepos=nextpos;
	      featureInd++;
	    }//while
	}//else
    }
  return 0;
}

int readTheModel(CRM114_DATABLOCK **db, FILE *fp)
{
  int whichblock = (*db)->cb.how_many_blocks-1;
  int readIn = 0;
  char* writeCharPtr = (char*)(&(*db)->data[0] + (*db)->cb.block[whichblock].start_offset);
  size_t datasize=0, nextpos=0, freepos=0;
    
  /////////////////////////////////////////// Read one integer svm_type
  fscanf(fp, " %d", &readIn);
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  memcpy(writeCharPtr+freepos,(char *)&readIn, datasize);
  freepos=nextpos;
	
  /////////////////// Read one integer kernel_type
  fscanf(fp, " %d", &readIn);
  int kernel_type = readIn;
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  memcpy(writeCharPtr+freepos,(char *)&readIn, datasize);
  freepos=nextpos;
  /////////////////// Read one integer how many class
  fscanf(fp, " %d", &readIn);
  int howManyClass = readIn;
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  memcpy(writeCharPtr+freepos,(char *)&readIn, datasize);
  freepos=nextpos;
	
  /////////////////// Read one integer the length of the model
  fscanf(fp, " %d", &readIn);
  int lengthOfModel = readIn;
  datasize=sizeof(int);
  nextpos=freepos+datasize;
  memcpy(writeCharPtr+freepos,(char *)&readIn, datasize);
  freepos=nextpos;
	
  /////////////////// Read n(n-1)/2 double values rho
  double readDo=0.0;
  float readFl = 0.0;
  int howmanyrho = howManyClass*(howManyClass-1)/2;
  datasize=sizeof(double);

  for(int i=0;i<howmanyrho;i++)
    {
      nextpos = freepos + datasize;
			
      fscanf(fp, " %f", &readFl);
      readDo  = (double) readFl;
      memcpy(writeCharPtr+freepos,(char *)&readDo, datasize);
      freepos=nextpos;
    }

  /////////////////// Read n int values labels
	
  datasize=sizeof(int);
  for(int i=0;i<howManyClass;i++)
    {
      nextpos = freepos + datasize;
      fscanf(fp, " %d", &readIn);
					
      memcpy(writeCharPtr+freepos,(char *)&readIn, datasize);
      freepos=nextpos;
    }

	
  /////////////////// Read n(n-1)/2 double values probA
  int howmanyprobA = howmanyrho;
	
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobA;i++)
    {
      nextpos = freepos + datasize;
      fscanf(fp, " %f", &readFl);
      readDo  = (double) readFl;
      memcpy(writeCharPtr+freepos,(char *)&readDo, datasize);
      freepos=nextpos;
    }

  /////////////////// Read n(n-1)/2 double values probB
  int howmanyprobB = howmanyrho;
	
  datasize=sizeof(double);
  for(int i=0;i<howmanyprobB;i++)
    {
      nextpos = freepos + datasize;
      fscanf(fp, " %f", &readFl);
      readDo  = (double) readFl;
      memcpy(writeCharPtr+freepos,(char *)&readDo, datasize);
      freepos=nextpos;
    }
	
  /////////////////// Read n int values nSV
  datasize=sizeof(int);
	
  for(int i=0;i<howManyClass;i++)
    {
      nextpos = freepos + datasize;
      fscanf(fp, " %d", &readIn);
			
      memcpy(writeCharPtr+freepos,(char *)&readIn, datasize);
      freepos=nextpos;
    }
	
  /////////////////////////////////////// Read sv_coef double values n * length of model 
	
	
  for(int i=0;i<lengthOfModel;i++)
    {
      for(int j=0;j<howManyClass-1;j++)
	{
	  datasize=sizeof(double);
	  nextpos = freepos + datasize;
	  fscanf(fp, " %f", &readFl);
	  readDo  = (double) readFl;
	  memcpy(writeCharPtr+freepos,(char *)&readDo, datasize);
	  freepos=nextpos;
	}

      if(kernel_type == PRECOMPUTED)
	{
	  datasize=sizeof(double);
	  nextpos = freepos + datasize;
	  fscanf(fp, " %f", &readFl);
	  readDo  = (double) readFl;
	  memcpy(writeCharPtr+freepos,(char *)&readDo, datasize);
	  freepos=nextpos;
		
	}
      else
	{
	  while(1)
	    {
	      datasize=sizeof(int);
	      nextpos = freepos + datasize;
	      fscanf(fp, " %d", &readIn);
	      memcpy(writeCharPtr+freepos,(char *)&readIn, datasize);
	      freepos=nextpos;
	      if(readIn == -1)
		break;
	      datasize=sizeof(double);
	      nextpos = freepos + datasize;
	      fscanf(fp, " %f", &readFl);
	      readDo  = (double) readFl;
	      memcpy(writeCharPtr+freepos,(char *)&readDo, datasize);
	      freepos=nextpos;
	    }
	}//else	
    }//for
	
  return 0;
}

// Read/write a DB's data blocks from/to a text file.

int crm114__libsvm_learned_write_text_fp(const CRM114_DATABLOCK *db,
					 FILE *fp)
{
  //Most of the function is the exact copy of the function in crmm114_hyperspace.c
  int b;		// block subscript (same as class subscript)
  int i;		// feature/sentinel subscript

  for (b = 0; b < db->cb.how_many_blocks-2; b++)
    {
      HYPERSPACE_FEATUREBUCKET *ht; // pointer to our current class known features
      int htlen;        //   how many known features in this class

      //   get a pointer to the known features hash table
      ht = (HYPERSPACE_FEATUREBUCKET *) (&db->data[0]
					 + db->cb.block[b].start_offset);
      //   how many valid hashes + sentinels are here?
      htlen =
	db->cb.class[b].documents
	+ db->cb.class[b].features;

      fprintf(fp, TN_BLOCK " %d\n", b);
      for (i = 0; i < htlen; i++)
	fprintf(fp, "%u\n", ht[i].hash);
    }
    
  //This is to write the block which stores the densification table.
    
  int whichblock = (db)->cb.how_many_blocks-2; //index of the second to the last block
  int *densePtr;
    
  int sentinel =-1;
     
  densePtr = (int*)(&(db)->data[0] + (db)->cb.block[whichblock].start_offset);
  long numofFeatures = (long)(db)->cb.block[whichblock].size_used / sizeof(int); 
    
  for (int j=0;j<numofFeatures;j++)
    {
      fprintf(fp, "%d\n", densePtr[j]);
    }
  fprintf(fp, "%d\n", sentinel);
    
    
  //Now write the model in the last block into a file.
   
  writeTheModel(db,fp);  

  return TRUE;			// writes always work, right?
}

// Doesn't realloc() *db.  Shouldn't need to...
int crm114__libsvm_learned_read_text_fp(CRM114_DATABLOCK **db, FILE *fp)
{//This is also pretty similar to the function in crm114_hyperspace.c
	
  int b;		// block subscript (same as class subscript)
  int chkb;		// block subscript read from text file
  int i;		// feature/sentinel subscript

  for (b = 0; b < (*db)->cb.how_many_blocks-2; b++)
    {
      HYPERSPACE_FEATUREBUCKET *ht; // pointer to our current class known features
      int htlen;        //   how many known features in this class
	
      //   get a pointer to the known features hash table
      ht = (HYPERSPACE_FEATUREBUCKET *) (&(*db)->data[0]
					 + (*db)->cb.block[b].start_offset);
      //   how many valid hashes + sentinels are here?
      htlen =
	(*db)->cb.class[b].documents
	+ (*db)->cb.class[b].features;
	
      if (fscanf(fp, " " TN_BLOCK " %d", &chkb) != 1 || chkb != b)
	return FALSE;
      for (i = 0; i < htlen; i++)
	if (fscanf(fp, " %u", &ht[i].hash) != 1)
	  return FALSE;
    }
    
  //Read in the densification table
    
  int whichblock = (*db)->cb.how_many_blocks-2; //index of the second to the last block
  int *densePtr;
  int sentinel =-1;
  long numofFeatures=0;
  int readIn=0;

  densePtr = (int*)(&(*db)->data[0] + (*db)->cb.block[whichblock].start_offset);
    
    
  while(readIn != sentinel)
    {
      fscanf(fp, " %d", &readIn);
      densePtr[numofFeatures]=readIn;
      numofFeatures++;
    }
    
  //Now read in the last block which contains the model structure
    
  readTheModel(db,fp);	
		
  return TRUE;
}

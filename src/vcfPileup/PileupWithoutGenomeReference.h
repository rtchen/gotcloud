#ifndef __PILEUP_WITHOUT_GENOME_REFERENCE_H__
#define __PILEUP_WITHOUT_GENOME_REFERENCE_H__

#include "Pileup.h"
#include "InputFile.h"

struct Region
{
  std::string chrom;
  uint32_t start;
  uint32_t end;
  vector<uint32_t> *positions;
  uint32_t currentPosition;
};

template <class PILEUP_TYPE, 
  class FUNC_CLASS = defaultPileup<PILEUP_TYPE> >
  class PileupWithoutGenomeReference:public Pileup<PILEUP_TYPE, FUNC_CLASS>{

 public:
 PileupWithoutGenomeReference(bool addDelAsBase = false, 
			      bool inputVCFFileIsGZipped = false,
			      bool outputVCFFileIsGZipped = false, 
			      const FUNC_CLASS& fp = FUNC_CLASS());

 PileupWithoutGenomeReference(int window,
			      bool addDelAsBase = false,  
			      bool inputVCFFileIsGZipped = false,
			      bool outputVCFFileIsGZipped = false, 
			      const FUNC_CLASS& fp = FUNC_CLASS());
           
 virtual int processFile(const std::string& bamFileName,  
			 const std::string& outputVCFFileName,
			 uint16_t excludeFlag = 0x0704, 
			 //uint16_t excludeFlag = 0, 
			 uint16_t includeFlag = 0);        

 virtual int processFile(const std::string& bamFileName, 
			 const std::string& bamIndexFileName, 
			 const std::string& inputVCFFileName,
			 const std::string& outputVCFFileName,
			 uint16_t excludeFlag = 0x0704,
			 //uint16_t excludeFlag = 0,
			 uint16_t includeFlag = 0);
                    
 virtual void processAlignment(SamRecord& record, Region* region);
    	                                             					                    
 void initLogGLMatrix();
    
 private:
 void resetElement(PILEUP_TYPE& element, int position);
	
 bool myAddDelAsBase;
 bool inputVCFFileIsGZipped;
 bool outputVCFFileIsGZipped;
 IFILE myOutputVCFFile;
 double*** myLogGLMatrix;
};

template <class PILEUP_TYPE, class FUNC_CLASS>
  PileupWithoutGenomeReference<PILEUP_TYPE, FUNC_CLASS>::PileupWithoutGenomeReference(										      bool addDelAsBase, 
																				      bool inputVCFFileIsGZipped,
																				      bool outputVCFFileIsGZipped, 
										      const FUNC_CLASS& fp)
  : 	Pileup<PILEUP_TYPE>(),
  myAddDelAsBase(addDelAsBase),
  inputVCFFileIsGZipped(inputVCFFileIsGZipped),
  outputVCFFileIsGZipped(outputVCFFileIsGZipped)
  {	
    initLogGLMatrix();
  }

template <class PILEUP_TYPE, class FUNC_CLASS>
  PileupWithoutGenomeReference<PILEUP_TYPE, FUNC_CLASS>::PileupWithoutGenomeReference(int window, 
										      bool addDelAsBase, 
										      bool inputVCFFileIsGZipped,
										      bool outputVCFFileIsGZipped, 
										      const FUNC_CLASS& fp)
  :	Pileup<PILEUP_TYPE>(window),
  myAddDelAsBase(addDelAsBase),
  inputVCFFileIsGZipped(inputVCFFileIsGZipped),
  outputVCFFileIsGZipped(outputVCFFileIsGZipped),
  myOutputVCFFile(NULL)
  {	
    initLogGLMatrix();
  }

template <class PILEUP_TYPE, class FUNC_CLASS>
  int PileupWithoutGenomeReference<PILEUP_TYPE, FUNC_CLASS>::processFile(const std::string& bamFileName,
									 const std::string& outputVCFFileName, 
									 uint16_t excludeFlag,
									 uint16_t includeFlag)
{
  myOutputVCFFile = ifopen(outputVCFFileName.c_str(), "w", outputVCFFileIsGZipped?InputFile::GZIP:InputFile::DEFAULT);
  if(myOutputVCFFile==NULL){std::cerr << "failed to read " << outputVCFFileName.c_str() <<"\n"; exit(1);}
  std::string tempStr("#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\t");
  tempStr.append(outputVCFFileName.c_str());
  tempStr.append("\n");
  ifwrite(myOutputVCFFile, tempStr.c_str(), tempStr.length());
  
  Pileup<PILEUP_TYPE>::processFile(bamFileName);
		
  ifclose(myOutputVCFFile);
   
  return(0);  
}

template <class PILEUP_TYPE, class FUNC_CLASS>
  int PileupWithoutGenomeReference<PILEUP_TYPE, FUNC_CLASS>::processFile(const std::string& bamFileName, 
									 const std::string& bamIndexFileName, 
									 const std::string& inputVCFFileName, 
									 const std::string& outputVCFFileName, 
									 uint16_t excludeFlag,
									 uint16_t includeFlag)
{
  myOutputVCFFile = ifopen(outputVCFFileName.c_str(), "w", outputVCFFileIsGZipped?InputFile::GZIP:InputFile::DEFAULT);
  if(myOutputVCFFile==NULL){std::cerr << "failed to read " << myOutputVCFFile <<"\n"; exit(1);}
  std::string tempStr("#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\t");
  tempStr.append(outputVCFFileName.c_str());
  tempStr.append("\n");
  ifwrite(myOutputVCFFile, tempStr.c_str(), tempStr.length());
		    	
  //read through input VCF file to collect regions
  IFILE vcfIn = ifopen(inputVCFFileName.c_str(), "r", inputVCFFileIsGZipped?InputFile::GZIP:InputFile::DEFAULT);
  if(vcfIn==NULL){std::cerr << "failed to read " << inputVCFFileName <<"\n"; exit(1);}
  std::string field;
  std::string chromosome = "0";
  uint32_t position = 0;
  char c = ' ';
  vector<Region> regions;
  Region currentRegion;
  currentRegion.chrom = "0";
  currentRegion.start = 0;
  currentRegion.end = 0;
  currentRegion.positions = new vector<uint32_t>();
  currentRegion.currentPosition = 0;

  //fprintf(stderr,"bar %d %d\n",currentRegion.end-currentRegion.start,ifeof(vcfIn));
	
  while(!ifeof(vcfIn))
  {
    //fprintf(stderr,"bar %d\n",currentRegion.end-currentRegion.start);
      if((c=ifgetc(vcfIn))!='#')
      {
	//std::cerr << "foo\n";
	if ( ifeof(vcfIn) ) break;
	  field.append(&c, 1);
			
	  //read chromosome
	  while((c=ifgetc(vcfIn))!='\t')
	    {
	      field.append(&c, 1);
	    }
			
	  //std::cerr << "chrom:" << field << " ";
	  chromosome = field;
	  field.clear();
			
	  //read position
	  while((c=ifgetc(vcfIn))!='\t')
	    {
	      field.append(&c, 1);
	    }
			
	  //std::cerr << " pos:" << field << "\n";
	  position = (uint32_t) atoi(field.c_str());		
	  field.clear();
			
	  //std::cout << "X chrom:" << chromosome << " pos:" << position << "\n";

	  //decide to include region or not
	  if(chromosome!=currentRegion.chrom)
	    {
	      //add current region
	      Region newRegion;
	      newRegion.chrom = currentRegion.chrom;
	      newRegion.start = currentRegion.start;
	      newRegion.end = currentRegion.end;
	      newRegion.positions = currentRegion.positions;
	      newRegion.currentPosition = currentRegion.currentPosition;
	      regions.push_back(newRegion);
				
	      //create new current region
	      currentRegion.chrom=chromosome;
	      currentRegion.start=position;
	      currentRegion.end=position;
	      currentRegion.positions = new vector<uint32_t>();
	      currentRegion.positions->push_back(position);
	      currentRegion.currentPosition = 0;		
	    }
	  else
	    {
	      //extend region
	      if(position-currentRegion.end < 300)
		{
		  currentRegion.end = position;
		  currentRegion.positions->push_back(position);
		}
	      else
		{
		  //std::cerr << position << " minus " << currentRegion.end << "\n"; 
		  //add current region
		  Region newRegion;
		  newRegion.chrom = currentRegion.chrom;
		  newRegion.start = currentRegion.start;
		  newRegion.end = currentRegion.end;
		  newRegion.positions = currentRegion.positions;
		  regions.push_back(newRegion);
					
		  //create new current region
		  currentRegion.chrom=chromosome;
		  currentRegion.start=position;
		  currentRegion.end=position;
		  currentRegion.positions = new vector<uint32_t>();
		  currentRegion.positions->push_back(position);
					
		}	
	    }		
      }
		
      //read rest of line
      while((c=ifgetc(vcfIn))!='\n')
	{
	}
  }

  //fprintf(stderr,"foo %d\n",currentRegion.end-currentRegion.start);
			
  // add the last region
  if(currentRegion.end-currentRegion.start > 0) 
    {
      Region newRegion;
      newRegion.chrom = currentRegion.chrom;
      newRegion.start = currentRegion.start;
      newRegion.end = currentRegion.end;
      newRegion.positions = currentRegion.positions;
      regions.push_back(newRegion);
    }

  //printf(stderr,"foo %d\n",currentRegion.end-currentRegion.start);

  /*
  //std::cout << "size " << regions.size()<<"\n";
  for (uint i=1; i<regions.size(); ++i)
  //for (uint i=86770; i<86771; ++i)
  {
    std::cerr << "chr" << regions.at(i).chrom.c_str() << ":" <<  regions.at(i).start << "-" << regions.at(i).end;
    std::cerr << " positions: " ;
		
    for (uint k=0; k<regions.at(i).positions->size(); ++k)	
      {
	std::cerr << regions.at(i).positions->at(k) << " ";
      }
		
    std::cerr  << "\n";
  }
  */

  SamFile samIn;
  SamFileHeader header;
  SamRecord record;
    
  if(!samIn.OpenForRead(bamFileName.c_str()))
    {
      fprintf(stderr, "%s\n", samIn.GetStatusMessage());
      return(samIn.GetStatus());
    }
    
  if(!samIn.ReadHeader(header))
    {
      fprintf(stderr, "%s\n", samIn.GetStatusMessage());
      return(samIn.GetStatus());
    }

  // read index file
  if (!samIn.ReadBamIndex(bamIndexFileName.c_str()))
    {
      std::cerr << "samin status" << samIn.GetStatus() <<"\n";
      return(samIn.GetStatus());  	
    }
	
  // The file needs to be sorted by coordinate.
  samIn.setSortedValidation(SamFile::COORDINATE);

  // Iterate over selected regions
  for (uint i=1; i<regions.size(); ++i)
    //for (uint i=86770; i<86771; ++i)
    {
      int lastReadAlignmentStart = 0;
      Region currentRegion = regions.at(i);
      if(!samIn.SetReadSection(header.getReferenceID(currentRegion.chrom.c_str()), currentRegion.start-1, currentRegion.end))
	{							
	  //std::cerr << "empty chrom:" << regions.at(i).chrom << ":" <<  regions.at(i).start << "-" << regions.at(i).end << "\n";
	}
      else
	{
	  //std::cerr << "reading chrom:" << regions.at(i).chrom << ":" <<  regions.at(i).start << "-" << regions.at(i).end << "\n";
			
	  int count = 0;
	  // Iterate over all records
	  while (samIn.ReadRecord(header, record))
	    {
	      uint16_t flag = record.getFlag();
	      if(flag & excludeFlag)
		{
		  // This record has an excluded flag set, 
		  // so continue to the next one.
		  continue;
		}
	      if((flag & includeFlag) != includeFlag)
		{
		  // This record does not have all required flags set, 
		  // so continue to the next one.
		  continue;
		}
		        
	      if(record.get0BasedPosition()>=lastReadAlignmentStart)
		{
		  lastReadAlignmentStart = record.get0BasedPosition();
		  //std::cerr << "lastReadAlignmentStart " << lastReadAlignmentStart << "\n";
		  processAlignment(record, &currentRegion);
		}
		        		        	
	      ++count;
	    }
			
	  //std::cerr << " count:" << count << "\n";			
	}
    }	
   
  Pileup<PILEUP_TYPE>::flushPileup();

  int returnValue = 0;
  if(samIn.GetStatus() != SamStatus::NO_MORE_RECS)
    {
      // Failed to read a record.
      fprintf(stderr, "%s\n", samIn.GetStatusMessage());
      returnValue = samIn.GetStatus();
    }
 	
  ifclose(myOutputVCFFile);
   
  return(returnValue);  
}

template <class PILEUP_TYPE, class FUNC_CLASS>
  void PileupWithoutGenomeReference<PILEUP_TYPE, FUNC_CLASS>::processAlignment(SamRecord& record, Region* region)
{
  int refPosition = record.get0BasedPosition();
  int refID = record.getReferenceID();

  //std::cerr << "foo\n";

  // Flush any elements from the pileup that are prior to this record
  // since the file is sorted, we are done with those positions.
  Pileup<PILEUP_TYPE>::flushPileup(refID, refPosition);

  //std::cerr << "bar\n";
            
  // Loop through for each reference position covered by the record.
  // It is up to the PILEUP_TYPE to handle insertions/deletions, etc
  // that are related with the given reference position.
  //for(; refPosition <= record.get0BasedAlignmentEnd(); ++refPosition)
  //{
  //    Pileup<PILEUP_TYPE>::addAlignmentPosition(refPosition, record);
  //}
	
  //search for first location in region.positions that is >= the start position of the record
  while (region->positions->at(region->currentPosition)-1< (uint32_t)refPosition)
    {
      ++(region->currentPosition);
    }
	
  for (uint k=region->currentPosition; k<region->positions->size()&& region->positions->at(k)-1<=(uint32_t)record.get0BasedAlignmentEnd(); ++k)	
    {
      Pileup<PILEUP_TYPE>::addAlignmentPosition(region->positions->at(k)-1, record);
    }
}

template <class PILEUP_TYPE, class FUNC_CLASS>
  void PileupWithoutGenomeReference<PILEUP_TYPE, FUNC_CLASS>::resetElement(PILEUP_TYPE& element,
									   int position)
{
  element.reset(position, myOutputVCFFile, myAddDelAsBase, myLogGLMatrix);
}

template <class PILEUP_TYPE, class FUNC_CLASS>
  void PileupWithoutGenomeReference<PILEUP_TYPE, FUNC_CLASS>::initLogGLMatrix()
{
  int maxQualScore = 100;
  myLogGLMatrix = (double***)malloc(maxQualScore*sizeof(double**));
  for (int i=0; i<maxQualScore; ++i)
    {
      myLogGLMatrix[i] = (double**)malloc(10*sizeof(double*));
      for (int j=0; j<10; ++j)
	{	
	  myLogGLMatrix[i][j] = (double*)malloc(4*sizeof(double));
	}
    }
 
  std::string genotypes[10] = {"AA","AC","AG","AT","CC","CG","CT","GG","GT","TT"};
  char bases[4] = {'A','C','G','T'};
  double e[maxQualScore];
  char allele1;
  char allele2;
  char base;		

  for (int i=0; i<maxQualScore; ++i)
    {	
      e[i] = pow(10, -i/10.0)/3;
					
      for (int j=0; j<10; ++j)
    	{
	  for (int k=0; k<4; ++k)
	    {  	
	      allele1 = genotypes[j].c_str()[0];
	      allele2 = genotypes[j].c_str()[1];
	      base = bases[k];
				
	      if(allele1==allele2)
		{
		  myLogGLMatrix[i][j][k] = (base==allele1) ? log10(1-e[i]*3) : log10(e[i]);
		}
	      else
		{
		  myLogGLMatrix[i][j][k] = (base==allele1 || base==allele2) ? log10(0.5-e[i]) : log10(e[i]);
		}
	    }
    	}	
    }
}
#endif
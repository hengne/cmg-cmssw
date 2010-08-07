#ifndef PhysicsTools_PFCandProducer_TopProjector_
#define PhysicsTools_PFCandProducer_TopProjector_

// system include files
#include <iostream>
#include <memory>
#include <string>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "DataFormats/Provenance/interface/ProductID.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"
#include "DataFormats/TauReco/interface/PFTau.h"
#include "DataFormats/JetReco/interface/PFJet.h"

#include "DataFormats/Candidate/interface/CandidateFwd.h"


/**\class TopProjector 
\brief 

\author Colin Bernet
\date   february 2008
*/

#include <iostream>


template< class Top, class Bottom>
class TopProjector : public edm::EDProducer {

 public:

  typedef std::vector<Top> TopCollection;
  typedef edm::Handle< std::vector<Top> > TopHandle;
  typedef std::vector<Bottom> BottomCollection;
  typedef edm::Handle< std::vector<Bottom> > BottomHandle;
  typedef edm::Ptr<Bottom> BottomPtr; 

  TopProjector(const edm::ParameterSet&);

  ~TopProjector() {};

  
  void produce(edm::Event&, const edm::EventSetup&);


 private:
 
  /// fills ancestors with ptrs to the PFCandidates that in
  /// one way or another contribute to the candidate pointed to by 
  /// candPtr
  void
    ptrToAncestor( reco::CandidatePtr candRef,
		   reco::CandidatePtrVector& ancestors,
		   const edm::ProductID& ancestorsID,
		   const edm::Event& iEvent ) const;

  /// ancestors is a RefToBase vector. For each object in this vector
  /// get the index and set the corresponding slot to true in the 
  /// masked vector
  void maskAncestors( const reco::CandidatePtrVector& ancestors,
		      std::vector<bool>& masked ) const;
    
  
  void processCollection( const edm::Handle< std::vector<Top> >& handle,
			  const edm::Handle< std::vector<Bottom> >& allPFCandidates ,
			  std::vector<bool>& masked,
			  const char* objectName,
			  const edm::Event& iEvent ) const; 

  void  printAncestors( const reco::CandidatePtrVector& ancestors,
			const edm::Handle< std::vector<Bottom> >& allPFCandidates ) const;


  /// verbose ?
  bool            verbose_;

  std::string     name_;
 
  /// input tag for the top (masking) collection
  edm::InputTag   inputTagTop_;

  /// input tag for the masked collection. 
  edm::InputTag   inputTagBottom_;
};




template< class Top, class Bottom>
TopProjector< Top, Bottom >::TopProjector(const edm::ParameterSet& iConfig) {

  using namespace edm;

  verbose_ = iConfig.getUntrackedParameter<bool>("verbose",false);
  name_ = iConfig.getUntrackedParameter<std::string>("name","No Name");
  inputTagTop_ = iConfig.getParameter<InputTag>("topCollection");
  inputTagBottom_ = iConfig.getParameter<InputTag>("bottomCollection");

  // will produce a collection of the unmasked candidates in the 
  // bottom collection 
  produces< std::vector<Bottom> >();
}


template< class Top, class Bottom >
void TopProjector< Top, Bottom >::produce(edm::Event& iEvent,
					  const edm::EventSetup& iSetup) {
  
  using namespace std;
  using namespace edm;
  using namespace reco;
  
  if( verbose_)
    cout<<"Event -------------------- "<<iEvent.id().event()<<endl;
  
  // get the various collections

  // Access the masking collection
  TopHandle tops;
  iEvent.getByLabel(  inputTagTop_, tops );


/*   if( !tops.isValid() ) { */
/*     std::ostringstream  err; */
/*     err<<"The top collection must be supplied."<<endl */
/*        <<"It is now set to : "<<inputTagTop_<<endl; */
/*     edm::LogError("PFPAT")<<err.str(); */
/*     throw cms::Exception( "MissingProduct", err.str()); */
/*   } */

  edm::ProductID topsID = tops.id();

  

  // Access the collection to
  // be masked by the other ones
  BottomHandle bottoms;
  iEvent.getByLabel(  inputTagBottom_, bottoms );

/*   if( !bottoms.isValid() ) { */
/*     std::ostringstream  err; */
/*     err<<"The bottom collection must be supplied."<<endl */
/*        <<"It is now set to : "<<inputTagBottom_<<endl; */
/*     edm::LogError("PFPAT")<<err.str(); */
/*     throw cms::Exception( "MissingProduct", err.str()); */
/*   } */

  edm::ProductID bottomsID = bottoms.id();

 
  if(verbose_) {
    const Provenance& topProv = iEvent.getProvenance(tops.id());
    const Provenance& bottomProv = iEvent.getProvenance(bottoms.id());

    cout<<"Top projector: event "<<iEvent.id().event()<<endl;
    cout<<"Inputs --------------------"<<endl;
    cout<<"Top      :  "
	<<tops.id()<<"\t"<<tops->size()<<endl
	<<topProv.branchDescription()<<endl
	<<"Bottom   :  "
	<<bottoms.id()<<"\t"<<bottoms->size()<<endl
	<<bottomProv.branchDescription()<<endl;
  }


  // output collection of objects,
  // selected from the Bottom collection

  auto_ptr< BottomCollection >
    pBottomOutput( new BottomCollection );
  
  // mask for each bottom object.
  // at the beginning, all bottom objects are unmasked.
  std::vector<bool> masked( bottoms->size(), false);
    


  processCollection( tops, bottoms, masked, name_.c_str(), iEvent );


  const BottomCollection& inCands = *bottoms;

  if(verbose_)
    cout<<" Remaining candidates in the bottom collection ------ "<<endl;
  
  for(unsigned i=0; i<inCands.size(); i++) {
    
    if(masked[i]) {
      if(verbose_)
	cout<<"X "<<i<<" "<<inCands[i]<<endl;
      continue;
    }
    else {
      if(verbose_)
	cout<<"O "<<i<<" "<<inCands[i]<<endl;

      pBottomOutput->push_back( inCands[i] );
      BottomPtr motherPtr( bottoms, i );
      pBottomOutput->back().setSourceCandidatePtr(motherPtr); 
    }
  }

  iEvent.put( pBottomOutput );
}



template< class Top, class Bottom > 
void TopProjector< Top, Bottom >::processCollection( const edm::Handle< std::vector<Top> >& tops,
						     const edm::Handle< std::vector<Bottom> >& bottoms ,
						     std::vector<bool>& masked,
						     const char* objectName,
						     const edm::Event& iEvent) const {

  if( tops.isValid() && bottoms.isValid() ) {
    const std::vector<Top>& topCollection = *tops;
    
    if(verbose_) 
      std::cout<<"******* TopProjector "<<objectName
	       <<" size = "<<topCollection.size()<<" ******** "<<std::endl;
    
    for(unsigned i=0; i<topCollection.size(); i++) {
      
      
      edm::Ptr<Top>   ptr( tops, i);
      reco::CandidatePtr basePtr( ptr );
 
      
      reco::CandidatePtrVector ancestors;
      ptrToAncestor( basePtr,
		     ancestors,
		     bottoms.id(), 
		     iEvent );
      
      if(verbose_) {
/* 	std::cout<<"\t"<<objectName<<" "<<i */
/* 		 <<" pt,eta,phi = " */
/* 		 <<basePtr->pt()<<"," */
/* 		 <<basePtr->eta()<<"," */
/* 		 <<basePtr->phi()<<std::endl; */
	
	std::cout<<"\t"<<topCollection[i]<<std::endl;
	printAncestors( ancestors, bottoms );
      }
  
      maskAncestors( ancestors, masked );
    }
  }

}


template< class Top, class Bottom >
void  TopProjector<Top,Bottom>::printAncestors( const reco::CandidatePtrVector& ancestors,
				      const edm::Handle< std::vector<Bottom> >& allPFCandidates ) const {
  

  std::vector<Bottom> pfs = *allPFCandidates;

  for(unsigned i=0; i<ancestors.size(); i++) {

    edm::ProductID id = ancestors[i].id();
    assert( id == allPFCandidates.id() );
 
    unsigned index = ancestors[i].key();
    assert( index < pfs.size() );
    
    std::cout<<"\t\t"<<pfs[index]<<std::endl;
  }
}



template< class Top, class Bottom >
void
TopProjector<Top,Bottom>::ptrToAncestor( reco::CandidatePtr candPtr,
					 reco::CandidatePtrVector& ancestors,
					 const edm::ProductID& ancestorsID,
					 const edm::Event& iEvent) const {

  
  using namespace std;
  using namespace edm;
  using namespace reco;


  unsigned nSources = candPtr->numberOfSourceCandidatePtrs();

  if(verbose_) {
    const Provenance& hereProv = iEvent.getProvenance(candPtr.id());

    cout<<"going down from "<<candPtr.id()
	<<"/"<<candPtr.key()<<" #mothers "<<nSources
	<<" ancestor id "<<ancestorsID<<endl
	<<hereProv.branchDescription()<<endl;
  }  

  for(unsigned i=0; i<nSources; i++) {
    
    CandidatePtr mother = candPtr->sourceCandidatePtr(i);
    if( verbose_ ) {
/*       const Provenance& motherProv = iEvent.getProvenance(mother.id()); */
      cout<<"  mother id "<<mother.id()<<endl;
    }
    if(  mother.id() != ancestorsID ) {
      // the mother is not yet at lowest level
      ptrToAncestor( mother, ancestors, ancestorsID, iEvent );
    }
    else {
      // adding mother to the list of ancestors
      ancestors.push_back( mother ); 
    }
  }
}




template< class Top, class Bottom >
void TopProjector<Top,Bottom>::maskAncestors( const reco::CandidatePtrVector& ancestors,
					 std::vector<bool>& masked ) const {
  
  using namespace std;
  using namespace edm;
  using namespace reco;
  
  for(unsigned i=0; i<ancestors.size(); i++) {
    unsigned index = ancestors[i].key();
    assert( index<masked.size() );
    
    //     if(verbose_) {
    //       ProductID id = ancestors[i].id();
    //       cout<<"\tmasking "<<index<<", ancestor "<<id<<"/"<<index<<endl;
    //     }
    masked[index] = true;
  }
}


#endif

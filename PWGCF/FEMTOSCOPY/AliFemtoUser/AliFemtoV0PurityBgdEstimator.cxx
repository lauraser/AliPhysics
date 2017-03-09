///
/// \file AliFemtoV0PurityBgdEstimator.cxx
///

#include "AliFemtoV0PurityBgdEstimator.h"


const float DFLT_MinvMin = 0.0, DFLT_MinvMax = 1.0;
const int DFLT_NbinsMinv = 200;



//____________________________
AliFemtoV0PurityBgdEstimator::AliFemtoV0PurityBgdEstimator():
  AliFemtoV0PurityBgdEstimator("V0PurityBgdEst", DFLT_NbinsMinv, DFLT_MinvMin, DFLT_MinvMax)
{
// no-op
}


//____________________________
AliFemtoV0PurityBgdEstimator::AliFemtoV0PurityBgdEstimator(const char* title,
                                                           const int nbins,
                                                           const float MinvLo,
                                                           const float MinvHi):
  AliFemtoCorrFctn(),
  fTitle(title),
  fNbinsMinv(nbins),
  fMinvLow(MinvLo),
  fMinvHigh(MinvHi),
  fNumerator(nullptr),
  fDenominator(nullptr),
  fRatio(nullptr),
  fFemtoV0(nullptr),
  fFemtoV0TrackCut(nullptr)
{
  fNumerator = new TH1D(TString::Format("Num%s", fTitle.Data()),
                        "V0Purity - Sig+Bgd; M_{inv}(GeV/c^{2});",
                        nbins, MinvLo, MinvHi);
  fDenominator = new TH1D(TString::Format("Den%s", fTitle.Data()),
                          "V0Purity - Bgd; M_{inv}(GeV/c^{2});",
                          nbins, MinvLo, MinvHi);
  fRatio = new TH1D(TString::Format("Rat%s", fTitle.Data()),
                    "V0Purity - Ratio; M_{inv}(GeV/c^{2});",
                    nbins, MinvLo, MinvHi);

  // to enable error bar calculation...
  fNumerator->Sumw2();
  fDenominator->Sumw2();
  fRatio->Sumw2();

  fFemtoV0 = new AliFemtoV0();
  fFemtoV0TrackCut = new AliFemtoV0TrackCutNSigmaFilter();
}

//____________________________
AliFemtoV0PurityBgdEstimator::AliFemtoV0PurityBgdEstimator(const AliFemtoV0PurityBgdEstimator& aCorrFctn):
  AliFemtoCorrFctn(aCorrFctn),
  fTitle(aCorrFctn.fTitle),
  fNbinsMinv(aCorrFctn.fNbinsMinv),

  fNumerator(aCorrFctn.fNumerator ? new TH1D(*aCorrFctn.fNumerator) : nullptr),
  fDenominator(aCorrFctn.fDenominator ? new TH1D(*aCorrFctn.fDenominator) : nullptr),

  fFemtoV0(aCorrFctn.fFemtoV0 ? new AliFemtoV0(*aCorrFctn.fFemtoV0) : nullptr),
  fFemtoV0TrackCut(aCorrFctn.fFemtoV0TrackCut ? new AliFemtoV0TrackCutNSigmaFilter(*aCorrFctn.fFemtoV0TrackCut) : nullptr),

  fMinvLow(aCorrFctn.fMinvLow),
  fMinvHigh(aCorrFctn.fMinvHigh)
{
  // copy constructor
  fRatio = (aCorrFctn.fRatio) ? new TH1D(*aCorrFctn.fRatio) : nullptr;
}

//_________________________
AliFemtoV0PurityBgdEstimator& AliFemtoV0PurityBgdEstimator::operator=(const AliFemtoV0PurityBgdEstimator& aCorrFctn)
{
  // assignment operator
  if (this == &aCorrFctn) {
    return *this;
  }

  AliFemtoCorrFctn::operator=(aCorrFctn);

  fNbinsMinv = aCorrFctn.fNbinsMinv;
  fMinvLow = aCorrFctn.fMinvLow;
  fMinvHigh = aCorrFctn.fMinvHigh;

  fTitle = aCorrFctn.fTitle;

  if(fNumerator) delete fNumerator;
    fNumerator = new TH1D(*aCorrFctn.fNumerator);
  if(fDenominator) delete fDenominator;
    fDenominator = new TH1D(*aCorrFctn.fDenominator);
  if(fRatio) delete fRatio;
    fRatio = new TH1D(*aCorrFctn.fRatio);

  if(fFemtoV0) delete fFemtoV0;
    fFemtoV0 = new AliFemtoV0(*aCorrFctn.fFemtoV0);

  if(fFemtoV0TrackCut) delete fFemtoV0TrackCut;
    fFemtoV0TrackCut = new AliFemtoV0TrackCutNSigmaFilter(*aCorrFctn.fFemtoV0TrackCut);

  return *this;
}

//____________________________
AliFemtoV0PurityBgdEstimator* AliFemtoV0PurityBgdEstimator::Clone()
{
  return(new AliFemtoV0PurityBgdEstimator(*this));
}

//____________________________
AliFemtoV0PurityBgdEstimator::~AliFemtoV0PurityBgdEstimator()
{
  // destructor
  delete fNumerator;
  delete fDenominator;
  delete fRatio;
}

//____________________________
AliFmThreeVector<double> AliFemtoV0PurityBgdEstimator::ShiftMomentumToDCA(const AliFmHelix& tHelix, const AliFmThreeVector<double>& tMomentum, double tPathLengthToDCA)
{
  //Based on AliFmPhysicalHelix::MomentumAt(double S, double B)
  double xc = tHelix.XCenter();
  double yc = tHelix.YCenter();
  double rx = (tHelix.Y(tPathLengthToDCA)-yc)/(tHelix.Origin().y()-yc);
  double ry = (tHelix.X(tPathLengthToDCA)-xc)/(tHelix.Origin().x()-xc);
  return tMomentum.PseudoProduct(rx,ry,1.0);
}

//____________________________
void AliFemtoV0PurityBgdEstimator::UseCorrectedDaughterHelices(const AliFemtoTrack* tTrackPos, const AliFemtoTrack* tTrackNeg)
{
  //Set the positive and negative daughter momenta to the correct value at their DCA to each other
  //Set the DCA of the daughters to each other
  //Set the decay vertex of the V0
  //The UpdateV0 call below in BuildV0 will handle setting a number of other related attributes

  const AliFmPhysicalHelixD tHelixPosOG = tTrackPos->Helix();
  const AliFmPhysicalHelixD tHelixNegOG = tTrackNeg->Helix();

  //When the helix was built, it assumed that particles come directly from event vertex
  //This is not useful for me, so I build new ones with more correct (hopefully) origins
  const AliFmThreeVector<double> tOriginPos(tTrackPos->XatDCA(), tTrackPos->YatDCA(), tTrackPos->ZatDCA());
  const AliFmThreeVector<double> tOriginNeg(tTrackNeg->XatDCA(), tTrackNeg->YatDCA(), tTrackNeg->ZatDCA());

  const AliFmPhysicalHelixD tHelixPos(tHelixPosOG.Curvature(), tHelixPosOG.DipAngle(), tHelixPosOG.Phase(), tOriginPos, tHelixPosOG.H());
  const AliFmPhysicalHelixD tHelixNeg(tHelixNegOG.Curvature(), tHelixNegOG.DipAngle(), tHelixNegOG.Phase(), tOriginNeg, tHelixNegOG.H());

  pair<double, double> tPathLengthsAtDca = tHelixPos.PathLengths(tHelixNeg);

  AliFmThreeVector<double> tPositionAtDcaPos = tHelixPos.At(tPathLengthsAtDca.first);
  AliFmThreeVector<double> tPositionAtDcaNeg = tHelixNeg.At(tPathLengthsAtDca.second);

  AliFmThreeVector<double> tMomPosAtOrigin = tTrackPos->P();
  AliFmThreeVector<double> tMomNegAtOrigin = tTrackNeg->P();

  AliFmThreeVector<double> tMomPosAtDCA = ShiftMomentumToDCA(tHelixPos,tMomPosAtOrigin,tPathLengthsAtDca.first);
  fFemtoV0->SetmomPos(tMomPosAtDCA);
    fFemtoV0->SetmomPosX(tMomPosAtDCA.x());
    fFemtoV0->SetmomPosY(tMomPosAtDCA.y());
    fFemtoV0->SetmomPosZ(tMomPosAtDCA.z());

  AliFmThreeVector<double> tMomNegAtDCA = ShiftMomentumToDCA(tHelixNeg,tMomNegAtOrigin,tPathLengthsAtDca.second);
  fFemtoV0->SetmomNeg(tMomNegAtDCA);
    fFemtoV0->SetmomNegX(tMomNegAtDCA.x());
    fFemtoV0->SetmomNegY(tMomNegAtDCA.y());
    fFemtoV0->SetmomNegZ(tMomNegAtDCA.z());

  AliFmThreeVector<double> tDecayVertex = tPositionAtDcaPos;
  tDecayVertex += tPositionAtDcaNeg;
  tDecayVertex /= 2.0;

  double tDca = tHelixPos.Distance(tPositionAtDcaNeg);

  fFemtoV0->SetdcaV0Daughters(tDca);
  fFemtoV0->SetdecayVertexV0(tDecayVertex);
}


//____________________________
void AliFemtoV0PurityBgdEstimator::BuildV0(AliFemtoPair* aPair)
{
  AliFemtoTrack* tTrackPos = aPair->Track1()->Track();
  AliFemtoTrack* tTrackNeg = aPair->Track2()->Track();

  //Make sure positive and negative daughters have correct charge,
  //If not, switch them
  if(tTrackPos->Charge() < 0 && tTrackNeg->Charge()> 0)
  {
    AliFemtoTrack* tTmpTrack = tTrackPos;
    tTrackPos = tTrackNeg;
    tTrackNeg = new AliFemtoTrack(*tTmpTrack);
    delete tTmpTrack;
  }

  double tPrimVtx[3];
  tTrackPos->GetPrimaryVertex(tPrimVtx);
  fFemtoV0->SetprimaryVertex(AliFemtoThreeVector(tPrimVtx));

  UseCorrectedDaughterHelices(tTrackPos,tTrackNeg);

  double tDecayLengthV0 = (fFemtoV0->DecayVertexV0X()-tPrimVtx[0])*(fFemtoV0->DecayVertexV0X()-tPrimVtx[0]) +
                          (fFemtoV0->DecayVertexV0Y()-tPrimVtx[1])*(fFemtoV0->DecayVertexV0Y()-tPrimVtx[1]) +
                          (fFemtoV0->DecayVertexV0Z()-tPrimVtx[2])*(fFemtoV0->DecayVertexV0Z()-tPrimVtx[2]);
  tDecayLengthV0 = sqrt(tDecayLengthV0);
  fFemtoV0->SetdecayLengthV0(tDecayLengthV0);

  fFemtoV0->UpdateV0();

  TVector3 tPrimToSecVtxVec;
  tPrimToSecVtxVec.SetX(fFemtoV0->DecayVertexV0X()-tPrimVtx[0]);
  tPrimToSecVtxVec.SetY(fFemtoV0->DecayVertexV0Y()-tPrimVtx[1]);
  tPrimToSecVtxVec.SetZ(fFemtoV0->DecayVertexV0Z()-tPrimVtx[2]);

  TVector3 tMomV0;
  tMomV0.SetX(fFemtoV0->MomV0X());
  tMomV0.SetY(fFemtoV0->MomV0Y());
  tMomV0.SetZ(fFemtoV0->MomV0Z());

  double tDcaV0 = TMath::Abs(tPrimToSecVtxVec.Perp(tMomV0));
  double tCosPointing = TMath::Abs(tPrimToSecVtxVec.Dot(tMomV0)/(tPrimToSecVtxVec.Mag()*tMomV0.Mag()));

  fFemtoV0->SetdcaV0ToPrimVertex(tDcaV0);
  fFemtoV0->SetCosPointingAngle(tCosPointing);

//  fFemtoV0->SetdcaPosToPrimVertex(sqrt(pow(tTrackPos->ImpactD(),2)+pow(tTrackPos->ImpactZ(),2)));  //V0s apparently only use DcaXY here
  fFemtoV0->SetdcaPosToPrimVertex(tTrackPos->ImpactD());
  fFemtoV0->SetPosNSigmaTPCK(tTrackPos->NSigmaTPCK());
  fFemtoV0->SetPosNSigmaTPCPi(tTrackPos->NSigmaTPCPi());
  fFemtoV0->SetPosNSigmaTPCP(tTrackPos->NSigmaTPCP());
  fFemtoV0->SetPosNSigmaTOFK(tTrackPos->NSigmaTOFK());
  fFemtoV0->SetPosNSigmaTOFPi(tTrackPos->NSigmaTOFPi());
  fFemtoV0->SetPosNSigmaTOFP(tTrackPos->NSigmaTOFP());
  fFemtoV0->SetTPCNclsPos(tTrackPos->TPCncls());
  fFemtoV0->SetNdofPos(tTrackPos->TPCchi2());
  fFemtoV0->SetStatusPos(tTrackPos->Flags());
  fFemtoV0->SetidPos(tTrackPos->TrackId());

//  fFemtoV0->SetdcaNegToPrimVertex(sqrt(pow(tTrackNeg->ImpactD(),2)+pow(tTrackNeg->ImpactZ(),2)));  //V0s apparently only use DcaXY here
  fFemtoV0->SetdcaNegToPrimVertex(tTrackNeg->ImpactD());
  fFemtoV0->SetNegNSigmaTPCK(tTrackNeg->NSigmaTPCK());
  fFemtoV0->SetNegNSigmaTPCPi(tTrackNeg->NSigmaTPCPi());
  fFemtoV0->SetNegNSigmaTPCP(tTrackNeg->NSigmaTPCP());
  fFemtoV0->SetNegNSigmaTOFK(tTrackNeg->NSigmaTOFK());
  fFemtoV0->SetNegNSigmaTOFPi(tTrackNeg->NSigmaTOFPi());
  fFemtoV0->SetNegNSigmaTOFP(tTrackNeg->NSigmaTOFP());
  fFemtoV0->SetTPCNclsNeg(tTrackNeg->TPCncls());
  fFemtoV0->SetNdofNeg(tTrackNeg->TPCchi2());
  fFemtoV0->SetStatusNeg(tTrackNeg->Flags());
  fFemtoV0->SetidNeg(tTrackNeg->TrackId());
}


//____________________________
AliFemtoString AliFemtoV0PurityBgdEstimator::Report()
{
  // construct report
  TString report = "V0 Purity Background Estimator Report:\n";
  report += TString::Format("Number of entries in numerator:\t%E\n", fNumerator->GetEntries());
  report += TString::Format("Number of entries in denominator:\t%E\n", fDenominator->GetEntries());
  return AliFemtoString(report);
}

//______________________________
TList* AliFemtoV0PurityBgdEstimator::GetOutputList()
{
  // Prepare the list of objects to be written to the output
  TList *tOutputList = new TList();

  tOutputList->Add(fNumerator);
  tOutputList->Add(fDenominator);
  tOutputList->Add(fRatio);

  return tOutputList;
}

//_________________________
void AliFemtoV0PurityBgdEstimator::Finish()
{
  fRatio->Divide(fNumerator, fDenominator, 1.0, 1.0);
}

//____________________________
void AliFemtoV0PurityBgdEstimator::Write()
{
  // Write out neccessary objects
  fNumerator->Write();
  fDenominator->Write();
  fRatio->Write();
}


//____________________________
void AliFemtoV0PurityBgdEstimator::AddRealPair(AliFemtoPair* aPair)
{
  // add true pair
  BuildV0(aPair);
  if (!fFemtoV0TrackCut->Pass(fFemtoV0)) return;

  double tMinv;
  short tV0Type = fFemtoV0TrackCut->GetParticleType();
  if(tV0Type==AliFemtoV0TrackCut::kLambda || tV0Type==AliFemtoV0TrackCut::kLambdaMC) tMinv = fFemtoV0->MassLambda();
  else if(tV0Type==AliFemtoV0TrackCut::kAntiLambda || tV0Type==AliFemtoV0TrackCut::kAntiLambdaMC) tMinv = fFemtoV0->MassAntiLambda();
  else if(tV0Type==AliFemtoV0TrackCut::kK0s || tV0Type==AliFemtoV0TrackCut::kK0sMC) tMinv = fFemtoV0->MassK0Short();
  else tMinv = 0.;

  fNumerator->Fill(tMinv);
}

//____________________________
void AliFemtoV0PurityBgdEstimator::AddMixedPair(AliFemtoPair* aPair)
{
  // add mixed (background) pair
  BuildV0(aPair);
  if (!fFemtoV0TrackCut->Pass(fFemtoV0)) return;

  double weight = 1.0;
  double tMinv;
  short tV0Type = fFemtoV0TrackCut->GetParticleType();
  if(tV0Type==AliFemtoV0TrackCut::kLambda || tV0Type==AliFemtoV0TrackCut::kLambdaMC) tMinv = fFemtoV0->MassLambda();
  else if(tV0Type==AliFemtoV0TrackCut::kAntiLambda || tV0Type==AliFemtoV0TrackCut::kAntiLambdaMC) tMinv = fFemtoV0->MassAntiLambda();
  else if(tV0Type==AliFemtoV0TrackCut::kK0s || tV0Type==AliFemtoV0TrackCut::kK0sMC) tMinv = fFemtoV0->MassK0Short();
  else tMinv = 0.;

  fDenominator->Fill(tMinv,weight);

}




/**************************************************************************
 * Copyright(c) 1998-2010, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/////////////////////////////////////////////////////////////
//
// Class to handle systematic errors for charm hadrons
//
// Usage:
// AliHFSystEff syst(DECAY); // DECAY = 1 for D0, 2, for D+, 3 for D* 
// syst.DrawErrors(); // to see a plot of the error contributions
// syst.GetTotalSystErr(pt); // to get the total err at pt 
//
// Author: A.Dainese, andrea.dainese@pd.infn.it
/////////////////////////////////////////////////////////////

#include <TStyle.h>
#include <TGraphAsymmErrors.h>
#include <TMath.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TLegend.h>

#include "AliHFSystErr.h"


ClassImp(AliHFSystErr)

//--------------------------------------------------------------------------
AliHFSystErr::AliHFSystErr(const Char_t* name, const Char_t* title) : 
TNamed(name,title),
fNorm(0),
fRawYield(0),
fTrackingEff(0),
fBR(0),
fCutsEff(0),
fPIDEff(0),
fMCPtShape(0),
fPartAntipart(0)
{
  //
  // Default Constructor
  //
}
//--------------------------------------------------------------------------
AliHFSystErr::AliHFSystErr(Int_t decay,const Char_t* name, const Char_t* title) : 
TNamed(name,title),
fNorm(0),
fRawYield(0),
fTrackingEff(0),
fBR(0),
fCutsEff(0),
fPIDEff(0),
fMCPtShape(0),
fPartAntipart(0)
{
  //
  // Standard Constructor
  //

  switch(decay) {
  case 1: // D0->Kpi
    InitD0toKpi();
    break;
  case 2: // D+->Kpipi
    InitDplustoKpipi();
    break;
  case 3: // D*->D0pi
    InitDstartoD0pi();
    break;
  default:
    printf("Invalid decay type: %d\n",decay);
    break;
  }


}
//--------------------------------------------------------------------------
AliHFSystErr::~AliHFSystErr() {
  //  
  // Default Destructor
  //

  if(fNorm)         { delete fNorm; fNorm=0; }
  if(fRawYield)     { delete fRawYield; fRawYield=0; }
  if(fTrackingEff)  { delete fTrackingEff; fTrackingEff=0; }
  if(fBR)           { delete fBR; fBR=0; }
  if(fCutsEff)      { delete fCutsEff; fCutsEff=0; }
  if(fPIDEff)       { delete fPIDEff; fPIDEff=0; }
  if(fMCPtShape)    { delete fMCPtShape; fMCPtShape=0; }
  if(fPartAntipart) { delete fPartAntipart; fPartAntipart=0; }

}
//--------------------------------------------------------------------------
void AliHFSystErr::InitD0toKpi() {
  // 
  // D0->Kpi syst errors. Responsible: A. Rossi
  //
  
  // Normalization
  fNorm = new TH1F("fNorm","fNorm",20,0,20);
  for(Int_t i=4;i<=20;i++) fNorm->SetBinContent(i,0.10); // 10% error on sigmaV0and

  // Branching ratio 
  fBR = new TH1F("fBR","fBR",20,0,20);
  for(Int_t i=4;i<=20;i++) fBR->SetBinContent(i,0.012); // 1.2% PDG2010

  // Tracking efficiency
  fTrackingEff = new TH1F("fTrackingEff","fTrackingEff",20,0,20);
  for(Int_t i=4;i<=20;i++) fTrackingEff->SetBinContent(i,0.02); // 2% (1% per track)

  // Raw yield extraction
  fRawYield = new TH1F("fRawYield","fRawYield",20,0,20);
  fRawYield->SetBinContent(1,1);
  fRawYield->SetBinContent(2,1);
  fRawYield->SetBinContent(3,0.15);
  for(Int_t i=4;i<=20;i++) fRawYield->SetBinContent(i,0.06);

  // Cuts efficiency (from cuts variation)
  fCutsEff = new TH1F("fCutsEff","fCutsEff",20,0,20);
  for(Int_t i=1;i<=20;i++) fCutsEff->SetBinContent(i,0.10); // 10%

  // PID efficiency (from PID/noPID)
  fPIDEff = new TH1F("fPIDEff","fPIDEff",20,0,20);
  for(Int_t i=1;i<=20;i++) fPIDEff->SetBinContent(i,0.03); // 3%
  fPIDEff->SetBinContent(4,0.15); // 15%

  // MC dN/dpt
  fMCPtShape = new TH1F("fMCPtShape","fMCPtShape",20,0,20);
  for(Int_t i=1;i<=20;i++) fMCPtShape->SetBinContent(i,(Float_t)i*0.006);

  // particle-antiparticle
  fPartAntipart = new TH1F("fPartAntipart","fPartAntipart",20,0,20);
  fPartAntipart->SetBinContent(1,1);
  fPartAntipart->SetBinContent(2,1);
  fPartAntipart->SetBinContent(3,0.20);
  for(Int_t i=4;i<=20;i++) fPartAntipart->SetBinContent(i,0.05);
  
  return;
}
//--------------------------------------------------------------------------
void AliHFSystErr::InitDplustoKpipi() {
  // 
  // D+->Kpipi syst errors. Responsible: R. Bala
  //

  return;
}
//--------------------------------------------------------------------------
void AliHFSystErr::InitDstartoD0pi() {
  // 
  // D*+->D0pi syst errors. Responsible: tbd
  //

  return;
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetCutsEffErr(Double_t pt) const {
  // 
  // Get error
  //

  Int_t bin=fCutsEff->FindBin(pt);

  return fCutsEff->GetBinContent(bin);
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetMCPtShapeErr(Double_t pt) const {
  // 
  // Get error
  //

  Int_t bin=fMCPtShape->FindBin(pt);

  return fMCPtShape->GetBinContent(bin);
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetSeleEffErr(Double_t pt) const {
  // 
  // Get error
  //

  Double_t err=GetCutsEffErr(pt)*GetCutsEffErr(pt)+GetMCPtShapeErr(pt)*GetMCPtShapeErr(pt);

  return TMath::Sqrt(err);
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetPIDEffErr(Double_t pt) const {
  // 
  // Get error
  //

  Int_t bin=fPIDEff->FindBin(pt);

  return fPIDEff->GetBinContent(bin);
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetTrackingEffErr(Double_t pt) const {
  // 
  // Get error
  //

  Int_t bin=fTrackingEff->FindBin(pt);

  return fTrackingEff->GetBinContent(bin);
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetRawYieldErr(Double_t pt) const {
  // 
  // Get error
  //

  Int_t bin=fRawYield->FindBin(pt);

  return fRawYield->GetBinContent(bin);
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetPartAntipartErr(Double_t pt) const {
  // 
  // Get error
  //

  Int_t bin=fPartAntipart->FindBin(pt);

  return fPartAntipart->GetBinContent(bin);
}
//--------------------------------------------------------------------------
Double_t AliHFSystErr::GetTotalSystErr(Double_t pt,Double_t feeddownErr) const {
  // 
  // Get total syst error (except norm. error)
  //

  Double_t err=0.;

  if(fRawYield) err += GetRawYieldErr(pt)*GetRawYieldErr(pt);
  if(fTrackingEff) err += GetTrackingEffErr(pt)*GetTrackingEffErr(pt);
  if(fBR) err += GetBRErr()*GetBRErr();
  if(fCutsEff) err += GetCutsEffErr(pt)*GetCutsEffErr(pt);
  if(fPIDEff) err += GetPIDEffErr(pt)*GetPIDEffErr(pt);
  if(fMCPtShape) err += GetMCPtShapeErr(pt)*GetMCPtShapeErr(pt);
  if(fPartAntipart) err += GetPartAntipartErr(pt)*GetPartAntipartErr(pt);

  err += feeddownErr*feeddownErr;

  return TMath::Sqrt(err);
}
//---------------------------------------------------------------------------
void AliHFSystErr::DrawErrors(TGraphErrors *grErrFeeddown) const {
  //
  // Draw errors
  //
  gStyle->SetOptStat(0);

  TCanvas *cSystErr = new TCanvas("cSystErr","Systematic Errors",0,0,500,500);
  cSystErr->SetFillColor(0);

  TH2F *hFrame = new TH2F("hFrame","Systematic errors; p_{t} [GeV/c]; Relative Error",20,0,20,100,0,+1);
  hFrame->Draw();

  TLegend *leg=new TLegend(0.5,0.5,0.9,0.9);
  leg->SetFillStyle(0);
  leg->SetBorderSize(0);

  if(fNorm) {
    fNorm->SetFillColor(1);
    fNorm->SetFillStyle(3001);
    fNorm->Draw("same");
    leg->AddEntry(fNorm,"Normalization","f");
  }
  if(fTrackingEff) {
    fTrackingEff->SetFillColor(2);
    fTrackingEff->SetFillStyle(3004);
    fTrackingEff->Draw("same");
    leg->AddEntry(fTrackingEff,"Tracking efficiency","f");
  }
  if(fBR) {
    fBR->SetFillColor(6);
    fBR->SetFillStyle(3005);
    fBR->Draw("same");
    leg->AddEntry(fBR,"Branching ratio","f");
  }
  if(fRawYield) {
    fRawYield->SetLineColor(3);
    fRawYield->SetLineWidth(3);
    fRawYield->Draw("same");
    leg->AddEntry(fRawYield,"Inv. mass analysis","l");
  }
  if(fCutsEff) {
    fCutsEff->SetLineColor(4);
    fCutsEff->SetLineWidth(3);
    fCutsEff->Draw("same");
    leg->AddEntry(fCutsEff,"Cuts efficiency","l");
  }
  if(fPIDEff) {
    fPIDEff->SetLineColor(7);
    fPIDEff->SetLineWidth(3);
    fPIDEff->Draw("same");
    leg->AddEntry(fPIDEff,"PID efficiency","l");
  }
  if(fMCPtShape) {
    fMCPtShape->SetLineColor(8);
    fMCPtShape->SetLineWidth(3);
    fMCPtShape->Draw("same");
    leg->AddEntry(fMCPtShape,"MC p_{t} shape","l");
  }
  if(fPartAntipart) {
    fPartAntipart->SetLineColor(9);
    fPartAntipart->SetLineWidth(3);
    fPartAntipart->Draw("same");
    leg->AddEntry(fPartAntipart,"D = #bar{D}","l");
  }
  if(grErrFeeddown) {
    grErrFeeddown->SetFillColor(11);    
    grErrFeeddown->SetFillStyle(3001);
    grErrFeeddown->Draw("f");
    leg->AddEntry(grErrFeeddown,"Feed-down from B","f");
  }

  
  TH1F *hTotErr=new TH1F("hTotErr","",20,0,20);
  for(Int_t i=1;i<=20;i++) {
    Double_t pt=hTotErr->GetBinCenter(i);
    hTotErr->SetBinContent(i,GetTotalSystErr(pt));
  }
  hTotErr->SetLineColor(1);
  hTotErr->SetLineWidth(3);
  hTotErr->Draw("same");
  leg->AddEntry(hTotErr,"Total (excl. norm.)","l");
  

  leg->Draw();

  return;
}

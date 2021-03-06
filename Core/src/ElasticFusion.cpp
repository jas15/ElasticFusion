/*
 * This file is part of ElasticFusion.
 *
 * Copyright (C) 2015 Imperial College London
 * 
 * The use of the code within this file and all code within files that 
 * make up the software that is ElasticFusion is permitted for 
 * non-commercial purposes only.  The full terms and conditions that 
 * apply to the code within this file are detailed within the LICENSE.txt 
 * file and at <http://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/elastic-fusion-license/> 
 * unless explicitly stated.  By downloading this file you agree to 
 * comply with these terms.
 *
 * If you wish to use any of this code for commercial purposes then 
 * please email researchcontracts.engineering@imperial.ac.uk.
 *
 */
 
#include "ElasticFusion.h"
#include <stdio.h>
#include <iostream>
#include <fstream>

ElasticFusion::ElasticFusion(const int timeDelta,
                             const int countThresh,
                             const float errThresh,
                             const float covThresh,
                             const bool closeLoops,
                             const bool iclnuim,
                             const bool reloc,
                             const float photoThresh,
                             const float confidence,
                             const float depthCut,
                             const float icpThresh,
                             const bool fastOdom,
                             const float fernThresh,
                             const bool so3,
                             const bool frameToFrameRGB,
                             const std::string fileName)
 : frameToModel(RES_WIDTH,
                RES_HEIGHT,
                Intrinsics::getInstance().cx(),
                Intrinsics::getInstance().cy(),
                Intrinsics::getInstance().fx(),
                Intrinsics::getInstance().fy()),
   modelToModel(RES_WIDTH,
                RES_HEIGHT,
                Intrinsics::getInstance().cx(),
                Intrinsics::getInstance().cy(),
                Intrinsics::getInstance().fx(),
                Intrinsics::getInstance().fy()),
   //fern helps for global loop closure ? stored as a database p much
   //TODO what happens if we change 500?
   ferns(500, depthCut * 1000, photoThresh),
   saveFilename(fileName),
   //current location
   currPose(Eigen::Matrix4f::Identity()),
   tick(1),
   timeDelta(timeDelta),
   icpCountThresh(countThresh),
   icpErrThresh(errThresh),
   covThresh(covThresh),
   deforms(0),
   fernDeforms(0),
   consSample(20),
   resize(RES_WIDTH,
          RES_HEIGHT,
          RES_WIDTH / consSample,
          RES_HEIGHT / consSample),
   imageBuff(RES_HEIGHT / consSample, RES_WIDTH / consSample),
   consBuff(RES_HEIGHT / consSample, RES_WIDTH / consSample),
   timesBuff(RES_HEIGHT / consSample, RES_WIDTH / consSample),
   closeLoops(closeLoops),
   iclnuim(iclnuim),
   reloc(reloc),
   lost(false),
   lastFrameRecovery(false),
   trackingCount(0),
   maxDepthProcessed(20.0f),
   rgbOnly(false),
   icpWeight(icpThresh),
   pyramid(false), //TODO change here to remove pyramid
   fastOdom(fastOdom),
   confidenceThreshold(confidence),
   fernThresh(fernThresh),
   so3(so3),
   frameToFrameRGB(frameToFrameRGB),
   depthCutoff(depthCut)
{
  //NOTE initialisation tings
  createTextures();
  createCompute();
  createFeedbackBuffers();

  std::string filename = fileName;
  filename.append(".freiburg");

  std::ofstream file;
  file.open(filename.c_str(), std::fstream::out);
  file.close();

  Stopwatch::getInstance().setCustomSignature(12431231);
}

ElasticFusion::~ElasticFusion()
{
  //NOTE seems not to go here
  if(iclnuim)
  {
    savePly();
  }

  //Output deformed pose graph
  std::string fname = saveFilename;
  fname.append(".freiburg");

  //TICK("poseGraphSize"); //TODO remove
  std::ofstream f;
  f.open(fname.c_str(), std::fstream::out);

  //NOTE poseGraph is a vector
  for(size_t i = 0; i < poseGraph.size(); i++)
  {
    std::stringstream strs;

    double poseLogTimesDbl = iclnuim ? (double)poseLogTimes.at(i) : (double)poseLogTimes.at(i) / 1000000.0;
    strs << std::setprecision(6) << std::fixed << poseLogTimesDbl << " ";

    //NOTE these are just getters right? they don't really do any comp
    //second == second in pair (which is type Matrix4f)
    Eigen::Vector3f trans (poseGraph.at(i).second.topRightCorner(3, 1));
    Eigen::Matrix3f rot   (poseGraph.at(i).second.topLeftCorner(3, 3));

    f << strs.str() << trans(0) << " " << trans(1) << " " << trans(2) << " ";

    //sets currentCameraRotation = rot ? YEP basically doesn't give it an empty val to begin
    Eigen::Quaternionf currentCameraRotation(rot);

    f << currentCameraRotation.x() << " " << currentCameraRotation.y() << " " << currentCameraRotation.z() << " " << currentCameraRotation.w() << "\n";
  }

  f.close();
  //TOCK("poseGraphSize"); //TODO remove

  //TICK("deleteThings");
  //delete every texture
  for(std::map<std::string, GPUTexture*>::iterator it = textures.begin(); it != textures.end(); ++it)
  {
    delete it->second;
  }

  textures.clear();

  //delete every compute pack
  for(std::map<std::string, ComputePack*>::iterator it = computePacks.begin(); it != computePacks.end(); ++it)
  {
    delete it->second;
  }

  computePacks.clear();

  //delete every feedback buffer
  for(std::map<std::string, FeedbackBuffer*>::iterator it = feedbackBuffers.begin(); it != feedbackBuffers.end(); ++it)
  {
    delete it->second;
  }

  feedbackBuffers.clear();
  //TOCK("deleteThings");
}

//INIT
void ElasticFusion::createTextures()
{
  //NOTE createTextures takes 2.245ms. only called in init

  textures[GPUTexture::RGB]                   = new GPUTexture(RES_WIDTH,
                                                               RES_HEIGHT,
                                                               GL_RGBA,
                                                               GL_RGB,
                                                               GL_UNSIGNED_BYTE,
                                                               true,
                                                               true);

  textures[GPUTexture::DEPTH_RAW]             = new GPUTexture(RES_WIDTH,
                                                               RES_HEIGHT,
                                                               GL_LUMINANCE16UI_EXT,
                                                               GL_LUMINANCE_INTEGER_EXT,
                                                               GL_UNSIGNED_SHORT);

  textures[GPUTexture::DEPTH_FILTERED]        = new GPUTexture(RES_WIDTH,
                                                               RES_HEIGHT,
                                                               GL_LUMINANCE16UI_EXT,
                                                               GL_LUMINANCE_INTEGER_EXT,
                                                               GL_UNSIGNED_SHORT,
                                                               false,
                                                               true);

  textures[GPUTexture::DEPTH_METRIC]          = new GPUTexture(RES_WIDTH,
                                                               RES_HEIGHT,
                                                               GL_LUMINANCE32F_ARB,
                                                               GL_LUMINANCE,
                                                               GL_FLOAT);

  textures[GPUTexture::DEPTH_METRIC_FILTERED] = new GPUTexture(RES_WIDTH,
                                                               RES_HEIGHT,
                                                               GL_LUMINANCE32F_ARB,
                                                               GL_LUMINANCE,
                                                               GL_FLOAT);

  textures[GPUTexture::DEPTH_NORM]            = new GPUTexture(RES_WIDTH,
                                                               RES_HEIGHT,
                                                               GL_LUMINANCE,
                                                               GL_LUMINANCE,
                                                               GL_FLOAT,
                                                               true);
}

//INIT
void ElasticFusion::createCompute()
{
  //NOTE createCompute takes 8.335ms. only called on init
  std::string eV ("empty.vert");
  std::string qG ("quad.geom");

  computePacks[ComputePack::NORM]            = new ComputePack(loadProgramFromFile(eV, "depth_norm.frag", qG),
                                                               textures[GPUTexture::DEPTH_NORM]->texture);

  computePacks[ComputePack::FILTER]          = new ComputePack(loadProgramFromFile(eV, "depth_bilateral.frag", qG),
                                                               textures[GPUTexture::DEPTH_FILTERED]->texture);

  computePacks[ComputePack::METRIC]          = new ComputePack(loadProgramFromFile(eV, "depth_metric.frag", qG),
                                                               textures[GPUTexture::DEPTH_METRIC]->texture);

  computePacks[ComputePack::METRIC_FILTERED] = new ComputePack(loadProgramFromFile(eV, "depth_metric.frag", qG),
                                                               textures[GPUTexture::DEPTH_METRIC_FILTERED]->texture);
}

//INIT
FeedbackBuffer * ElasticFusion::helperCreateFeedbackBuffers() {
  //NOTE called only twice at init
  return new FeedbackBuffer(loadProgramGeomFromFile("vertex_feedback.vert", "vertex_feedback.geom"));
}

//INIT
void ElasticFusion::createFeedbackBuffers()
{
  //NOTE only called on initialisation
  feedbackBuffers[FeedbackBuffer::RAW]      = helperCreateFeedbackBuffers();
  feedbackBuffers[FeedbackBuffer::FILTERED] = helperCreateFeedbackBuffers();
}

//FRAME 1
void ElasticFusion::computeFeedbackBuffers()
{
  //NOTE computeFeedbackBuffers takes 2.245ms. only called in initial frame
  //TICK("feedbackBuffers");
  feedbackBuffers[FeedbackBuffer::RAW]->compute(textures[GPUTexture::RGB]->texture,
                                                textures[GPUTexture::DEPTH_METRIC]->texture,
                                                tick,
                                                maxDepthProcessed);

  feedbackBuffers[FeedbackBuffer::FILTERED]->compute(textures[GPUTexture::RGB]->texture,
                                                     textures[GPUTexture::DEPTH_METRIC_FILTERED]->texture,
                                                     tick,
                                                     maxDepthProcessed);
  //TOCK("feedbackBuffers");
}

//FRAME 1
void ElasticFusion::processInitialFrame()
{
  //NOTE is it worth changing the things here? only called in initial frame
  //in real life the first frame wouldn't even matter that much surely
  //initialise some things
  computeFeedbackBuffers();

  globalModel.initialise(*feedbackBuffers[FeedbackBuffer::RAW], *feedbackBuffers[FeedbackBuffer::FILTERED]);

  frameToModel.initFirstRGB(textures[GPUTexture::RGB]);
}

//--------------------------- FUNCTIONS THAT WILL MAKE A DIFFERENCE ----------------------------

//IMPROVED
bool ElasticFusion::denseEnough(const Img<Eigen::Matrix<unsigned char, 3, 1>> & img)
{
  //NOTE negligibly small @ 0.005ms.
  //NOTE so this basically always returns 0
  //pretty sure you can work out a MUCH better way to do it than to iterate through a loop 400+ times
  //maybe something like (rows*cols*0.75f)-(remRows*remCols + sum) > 0 then stop
  //think i did it - seems to be 2/3 as slow (0.01 down to 0.006) and counts less
  //maybe I could remove the mES ? thres in the cols loop?

  float maxEndScore(img.rows * img.cols); //add to this
  float threshold (maxEndScore * 0.75f);
  int sum (0);
  bool currPix (0);

  int i (0);
  while (maxEndScore > threshold && i < img.rows)
  {
    int j (0);
    while (maxEndScore > threshold && j < img.cols)
    {
      currPix = img.at<Eigen::Matrix<unsigned char, 3, 1>>(i, j)(0) > 0 &&
        img.at<Eigen::Matrix<unsigned char, 3, 1>>(i, j)(1) > 0 &&
        img.at<Eigen::Matrix<unsigned char, 3, 1>>(i, j)(2) > 0;
      sum += currPix;
      maxEndScore += currPix;
      j++;
    }
    i++;
    maxEndScore -= img.cols;
  }

  /*
  int sum = 0;

  for(int i = 0; i < img.rows; i++)
  {
    for(int j = 0; j < img.cols; j++)
    {
      sum += img.at<Eigen::Matrix<unsigned char, 3, 1>>(i, j)(0) > 0 &&
        img.at<Eigen::Matrix<unsigned char, 3, 1>>(i, j)(1) > 0 &&
        img.at<Eigen::Matrix<unsigned char, 3, 1>>(i, j)(2) > 0;
    }
  }
  return float(sum) / float(img.rows * img.cols) > 0.75f;
  */

  return maxEndScore > threshold;
}

//IMPROVE
//NOTE this is the original func. sequentialFrame gets called from here. called in MainController.cpp
//this will basically improve drastically when we improve the other functions below
void ElasticFusion::processFrame(const unsigned char * rgb,
    const unsigned short * depth,
    const int64_t & timestamp,
    const Eigen::Matrix4f * inPose,
    const float weightMultiplier,
    const bool bootstrap)
{
  TICK("Run"); //timer starts here

  TICK("WholePreProcess"); //10ms
  //Upload is a Pangolin thing so would that be the GUI aspect ?
  textures[GPUTexture::DEPTH_RAW]->texture->Upload(depth, GL_LUMINANCE_INTEGER_EXT, GL_UNSIGNED_SHORT);
  textures[GPUTexture::RGB]->texture->Upload(rgb, GL_RGB, GL_UNSIGNED_BYTE);

  TICK("Preprocess"); //5.8-8.3ms

  filterDepth(); //see line ~725. creates vector of uniforms based on current RGBD image then filters for max depth?
  metriciseDepth(); //above filterDepth. similar but not using resolution. also DEPTH_FILTERED

  TOCK("Preprocess");
  TOCK("WholePreProcess");

  //First run
  if(tick == 1)
  {
    processInitialFrame();
  }
  else
  {
    processSequentialFrame(inPose, weightMultiplier, bootstrap);
  }

  //TICK("pushItBack"); //0.003ms
  poseGraph.push_back(std::pair<unsigned long long int, Eigen::Matrix4f>(tick, currPose));
  poseLogTimes.push_back(timestamp);
  //TOCK("pushItBack");

  TICK("sampleGraph"); //2.2-4.3ms

  localDeformation.sampleGraphModel(globalModel.model());
  globalDeformation.sampleGraphFrom(localDeformation);

  TOCK("sampleGraph");

  TICK("predictFrame"); //9ms
  predict();

  if(!lost)
  {
    processFerns();
    tick++;
  }

  TOCK("predictFrame"); //9ms
  TOCK("Run"); //ends here
}

//----------------- should improve between here --------------------

//IMPROVE
//NOTE this is probably the most important function. called by processFrame
void ElasticFusion::processSequentialFrame(const Eigen::Matrix4f * inPose,
                                            const float weightMultiplier,
                                            const bool bootstrap)
{
  //TICK("processSeq"); //NOTE 120-160ms == 75% of Run
  Eigen::Matrix4f lastPose = currPose;
  bool trackingOk = true;

  //NOTE changed this. was bootstrap first
  //this big if stmt is about 40ms
  if (!inPose || bootstrap)
  {
    //TICK("autoFill");//0.3-1ms
    resize.image(indexMap.imageTex(), imageBuff);
    bool shouldFillIn = !denseEnough(imageBuff);
    //TOCK("autoFill");

    //initialise the odometrics for each frame
    //TICK("odomInit"); //8.9-11.9ms
    //WARNING initICP* must be called before initRGB*
    frameToModel.initICPModel(shouldFillIn ? &fillIn.vertexTexture : indexMap.vertexTex(),
        shouldFillIn ? &fillIn.normalTexture : indexMap.normalTex(),
        maxDepthProcessed, currPose);
    frameToModel.initRGBModel((shouldFillIn || frameToFrameRGB) ? &fillIn.imageTexture : indexMap.imageTex());

    frameToModel.initICP(textures[GPUTexture::DEPTH_FILTERED], maxDepthProcessed);
    frameToModel.initRGB(textures[GPUTexture::RGB]);
    //TOCK("odomInit");

    if(bootstrap)
    {
      assert(inPose);
      currPose = currPose * (*inPose);
    }

    Eigen::Vector3f trans = currPose.topRightCorner(3, 1);
    Eigen::Matrix<float, 3, 3, Eigen::RowMajor> rot = currPose.topLeftCorner(3, 3);

    //TICK("odom"); //27.8-31.6
    //NOTE function in Utils/RGBDOdometry
    frameToModel.getIncrementalTransformation(trans,
        rot,
        rgbOnly,
        icpWeight,
        pyramid,
        fastOdom,
        so3);
    //TOCK("odom");

    trackingOk = !reloc || frameToModel.lastICPError < 1e-04;

    if(reloc)
    {
      //NOTE maybe I can extract the things from the if else below ? probs wont make a difference
      //to performance but looks nicer
      if(!lost)
      {
        Eigen::MatrixXd covariance = frameToModel.getCovariance();
        //TODO same as the while below ??

        for(int i = 0; i < 6; i++)
        {
          if(covariance(i, i) > 1e-04)
          {
            trackingOk = false;
            break;
          }
        }

        if(!trackingOk)
        {
          trackingCount++;

          lost = trackingCount > 10;
          //NOTE was --
          //if(trackingCount > 10)
          //{
          //  lost = true;
          //}
        }
        else
        {
          trackingCount = 0;
        }
      }
      else if(lastFrameRecovery)
      {
        Eigen::MatrixXd covariance = frameToModel.getCovariance();
        //TODO can this just change to a while cmon
        //int i (0);
        //while (trackingOk && i < 6)
        //{
        //  trackingOk = covariance(i,i) > 1e-04;
        //  i++;
        //}

        for(int i = 0; i < 6; i++)
        {
          if(covariance(i, i) > 1e-04)
          {
            trackingOk = false;
            break;
          }
        }

        if(trackingOk)
        {
          lost = false;
          trackingCount = 0;
        }

        lastFrameRecovery = false;
      }
    }

    currPose.topRightCorner(3, 1) = trans;
    currPose.topLeftCorner(3, 3) = rot;
  }
  else
  {
    currPose = *inPose;
  }

  Eigen::Matrix4f diff = currPose.inverse() * lastPose;

  Eigen::Vector3f diffTrans = diff.topRightCorner(3, 1);
  Eigen::Matrix3f diffRot = diff.topLeftCorner(3, 3);

  //Weight by velocity
  //NOTE tried improving the small math but doesn't really get quicker
  float largest (0.01);
  float minWeight (0.5);
  float weighting (std::max(diffTrans.norm(), rodrigues2(diffRot).norm()));

  if(weighting > largest)
  {
    weighting = largest;
  }

  weighting = std::max(1.0f - (weighting / largest), minWeight) * weightMultiplier;

  std::vector<Ferns::SurfaceConstraint> constraints;

  predict(); //line 686

  Eigen::Matrix4f recoveryPose = currPose;

  std::vector<float> rawGraph;

  bool fernAccepted = false;

  //CLOSELOOPS TRUE
  if(closeLoops)
  {
    lastFrameRecovery = false;

    //TICK("Ferns::findFrame"); //2ms
    recoveryPose = ferns.findFrame(constraints,
        currPose,
        &fillIn.vertexTexture,
        &fillIn.normalTexture,
        &fillIn.imageTexture,
        tick,
        lost);
    //TOCK("Ferns::findFrame");

    //CLOSELOOPS TRUE
    //NOTE could do lazy eval? which is more likely to be false (implemented)
    //NOTE closeLoops more likely to be true. it's just set in header file. swapped order
    if(ferns.lastClosest != -1)
    {
      if(lost)
      {
        currPose = recoveryPose;
        lastFrameRecovery = true;
      }
      else
      {
        //printf("Constraint size = %d\n", constraints.size());
        //printf("Relative constraint size = %d\n", relativeCons.size());
        //Can we combine the two loops? only do each thing if they're less than .size()

        //TODO big loops - can we vectorize or does it already do it with compiler?
        for(size_t i = 0; i < constraints.size(); i++)
        {
          globalDeformation.addConstraint(constraints.at(i).sourcePoint,
              constraints.at(i).targetPoint,
              tick,
              ferns.frames.at(ferns.lastClosest)->srcTime,
              true);
        }

        for(size_t i = 0; i < relativeCons.size(); i++)
        {
          globalDeformation.addConstraint(relativeCons.at(i));
        }

        if(globalDeformation.constrain(ferns.frames, rawGraph, tick, true, poseGraph, true))
        {
          currPose = recoveryPose;

          poseMatches.push_back(PoseMatch(ferns.lastClosest, ferns.frames.size(), ferns.frames.at(ferns.lastClosest)->pose, currPose, constraints, true));

          fernDeforms += rawGraph.size() > 0;

          fernAccepted = true;
        }
      }
    } //end of ferns.lastClosest

    //If we didn't match to a fern
    //NOTE lazy eval. changed the order of this. was rawGraph @ end
    if(!rawGraph.size() && !lost) //CLOSELOOPS TRUE !rawGraph.size() used to be ...size() == 0
    {
      //TICK("closeLoops bit"); //~80ms
      //NOTE goes in here
      //Only predict old view, since we just predicted the current view for the ferns (which failed!)
      //TICK("IndexMap::INACTIVE"); //~5ms
      indexMap.combinedPredict(currPose, globalModel.model(), maxDepthProcessed,
          confidenceThreshold, 0, tick - timeDelta, timeDelta, IndexMap::INACTIVE);
      //TOCK("IndexMap::INACTIVE");

      //TICK("closeLoops::1"); //~10ms
      //WARNING initICP* must be called before initRGB*
      modelToModel.initICPModel(indexMap.oldVertexTex(), indexMap.oldNormalTex(), maxDepthProcessed, currPose);
      modelToModel.initRGBModel(indexMap.oldImageTex());

      modelToModel.initICP(indexMap.vertexTex(), indexMap.normalTex(), maxDepthProcessed);
      modelToModel.initRGB(indexMap.imageTex());

      Eigen::Vector3f trans = currPose.topRightCorner(3, 1);
      Eigen::Matrix<float, 3, 3, Eigen::RowMajor> rot = currPose.topLeftCorner(3, 3);
      //TOCK("closeLoops::1");

      //TICK("closeLoops::2"); //~60ms
      //NOTE func in RGBDOdometry
      modelToModel.getIncrementalTransformation(trans,
          rot,
          false,
          10,
          pyramid,
          fastOdom,
          false);
      //TOCK("closeLoops::2");

      Eigen::MatrixXd covar = modelToModel.getCovariance();
      bool covOk = true;

      //NOTE for vs while loop speed ??
      //NOTE improved wooo
      int i (0);
      while (covOk && i < 6)
      {
        covOk = covar(i,i) > covThresh;
        i++;
      }
      /*
      for(int i = 0; i < 6; i++)
      {
        if(covar(i, i) > covThresh)
        {
          covOk = false;
          break;
        }
      }
      */
      //TOCK("closeLoops bit");

      Eigen::Matrix4f estPose = Eigen::Matrix4f::Identity();

      estPose.topRightCorner(3, 1) = trans;
      estPose.topLeftCorner(3, 3) = rot;

      //TODO lazy eval?
      if(covOk && modelToModel.lastICPCount > icpCountThresh && modelToModel.lastICPError < icpErrThresh)
      {
        //NOTE doesn't go into here !!!
        //---------------------------------------------------------------------
        resize.vertex(indexMap.vertexTex(), consBuff);
        resize.time(indexMap.oldTimeTex(), timesBuff);

        //NOTE this is HUUUUGE each and every time - dont think it can change tho wah
        //could vectorize but wouldn't be worth it as no call to it
        for(int i = 0; i < consBuff.cols; i++)
        {
          for(int j = 0; j < consBuff.rows; j++)
          {
            if(consBuff.at<Eigen::Vector4f>(j, i)(2) > 0 &&
               consBuff.at<Eigen::Vector4f>(j, i)(2) < maxDepthProcessed &&
               timesBuff.at<unsigned short>(j, i) > 0)
            {
              Eigen::Vector4f worldRawPoint 
                      = currPose * Eigen::Vector4f(consBuff.at<Eigen::Vector4f>(j, i)(0),
                                                   consBuff.at<Eigen::Vector4f>(j, i)(1),
                                                   consBuff.at<Eigen::Vector4f>(j, i)(2),
                                                   1.0f);

              Eigen::Vector4f worldModelPoint 
                      = estPose * Eigen::Vector4f(consBuff.at<Eigen::Vector4f>(j, i)(0),
                                                  consBuff.at<Eigen::Vector4f>(j, i)(1),
                                                  consBuff.at<Eigen::Vector4f>(j, i)(2),
                                                  1.0f);

              constraints.push_back(Ferns::SurfaceConstraint(worldRawPoint, worldModelPoint));

              localDeformation.addConstraint(worldRawPoint,
                                             worldModelPoint,
                                             tick,
                                             timesBuff.at<unsigned short>(j, i),
                                             deforms == 0);
            }
          }
        }

        std::vector<Deformation::Constraint> newRelativeCons;

        //NOTE doesn't reach here
        //---------------------------------------------------------------------
        if(localDeformation.constrain(ferns.frames, rawGraph, tick, false, poseGraph, false, &newRelativeCons))
        {
          poseMatches.push_back(PoseMatch(ferns.frames.size() - 1, ferns.frames.size(), estPose, currPose, constraints, false));

          deforms += rawGraph.size() > 0;

          currPose = estPose;

          //NOTE why 3 ? magic numbers are poo
          for(size_t i = 0; i < newRelativeCons.size(); i += newRelativeCons.size() / 3)
          {
            relativeCons.push_back(newRelativeCons.at(i));
          }
        }//end of localDeformation.constrain
        //not here
        //---------------------------------------------------------------------
      }//end of covOK
    }//end of rawGraph.size
  }//end of closeLoops

  //TODO lazy eval
  if(!rgbOnly && trackingOk && !lost)
  {
    //TICK("indexMap");//3.1-3.7ms
    indexMap.predictIndices(currPose, tick, globalModel.model(), maxDepthProcessed, timeDelta);
    //TOCK("indexMap");

    globalModel.fuse(currPose, tick, textures[GPUTexture::RGB], textures[GPUTexture::DEPTH_METRIC],
        textures[GPUTexture::DEPTH_METRIC_FILTERED], indexMap.indexTex(), indexMap.vertConfTex(),
        indexMap.colorTimeTex(), indexMap.normalRadTex(), maxDepthProcessed, confidenceThreshold, weighting);

    //TICK("indexMap2");//3.1-4.2ms
    //TODO why are we doing the same thing twice ????? hm
    indexMap.predictIndices(currPose, tick, globalModel.model(), maxDepthProcessed, timeDelta);
    //TOCK("indexMap2");

    //If we're deforming we need to predict the depth again to figure out which
    //points to update the timestamp's of, since a deformation means a second pose update
    //this loop
    //NOTE was fernAccepted at end
    if(!fernAccepted && rawGraph.size()) //NOTE was size() > 0
    {
      indexMap.synthesizeDepth(currPose,
          globalModel.model(),
          maxDepthProcessed,
          confidenceThreshold,
          tick,
          tick - timeDelta,
          std::numeric_limits<unsigned short>::max());
    }

    globalModel.clean(currPose, tick, indexMap.indexTex(), indexMap.vertConfTex(), indexMap.colorTimeTex(),
        indexMap.normalRadTex(), indexMap.depthTex(), confidenceThreshold, rawGraph, timeDelta, maxDepthProcessed, fernAccepted);
  }
  //TOCK("processSeq");
}

//IMPROVE
//NOTE called by processFrame
void ElasticFusion::processFerns()
{
  //NOTE 1.4-4.6ms (2.9avg)
  ferns.addFrame(&fillIn.imageTexture, &fillIn.vertexTexture, &fillIn.normalTexture, currPose, tick, fernThresh);
}

//IMPROVE
void ElasticFusion::predict()
{
  //NOTE 5-10ms. 7 avg called twice per frame by the looks of things
  TICK("IndexMap::ACTIVE");
  //NOTE have changed this if block to lastFrameRecovery ? 0 : tick
  indexMap.combinedPredict(currPose,
                            globalModel.model(),
                            maxDepthProcessed,
                            confidenceThreshold,
                            lastFrameRecovery ? 0 : tick,
                            tick,
                            timeDelta,
                            IndexMap::ACTIVE);

  /*
  if(lastFrameRecovery)
  {
    indexMap.combinedPredict(currPose, globalModel.model(), maxDepthProcessed, confidenceThreshold, 0, tick, timeDelta, 
                              IndexMap::ACTIVE);
  }
  else
  {
    indexMap.combinedPredict(currPose, globalModel.model(), maxDepthProcessed, confidenceThreshold, tick, tick, timeDelta, 
                              IndexMap::ACTIVE);
  }
  */

  TICK("FillIn"); //1.2-5.4ms. 2.37avg
  fillIn.vertex(indexMap.vertexTex(), textures[GPUTexture::DEPTH_FILTERED], lost);
  fillIn.normal(indexMap.normalTex(), textures[GPUTexture::DEPTH_FILTERED], lost);
  fillIn.image(indexMap.imageTex(), textures[GPUTexture::RGB], lost || frameToFrameRGB);
  TOCK("FillIn");

  TOCK("IndexMap::ACTIVE");
}

//IMPROVE
//NOTE called in processFrame. Not really sure what this does
void ElasticFusion::metriciseDepth()
{
  TICK("metriciseDepth"); //0.7-1.2ms. avg 0.8
  std::vector<Uniform> uniforms;

  //NOTE there are a lot of push_back calls - what is being stored in here ? might be large data == slow
  uniforms.push_back(Uniform("maxD", depthCutoff));

  //NOTE 0.7ms for these two computes
  computePacks[ComputePack::METRIC]->compute(textures[GPUTexture::DEPTH_RAW]->texture, &uniforms);
  computePacks[ComputePack::METRIC_FILTERED]->compute(textures[GPUTexture::DEPTH_FILTERED]->texture, &uniforms);
  TOCK("metriciseDepth");
}

//IMPROVE
void ElasticFusion::filterDepth()
{
  TICK("filterDepth"); //5.2-7.2ms. avg 5.9
  std::vector<Uniform> uniforms;

  //NOTE again push_back dead af can't this be done a different way ?
  uniforms.push_back(Uniform("cols", (float)RES_WIDTH));
  uniforms.push_back(Uniform("rows", (float)RES_HEIGHT));
  uniforms.push_back(Uniform("maxD", depthCutoff));

  //NOTE this is 6ms ?!?!?!? why so diff. just bc there are 3 to do ? or wha. Might be the FILTER
  computePacks[ComputePack::FILTER]->compute(textures[GPUTexture::DEPTH_RAW]->texture, &uniforms);
  TOCK("filterDepth");
}

//--------------------------- and here ------------------------

//IMPROVE is there any point
Eigen::Vector3f ElasticFusion::rodrigues2(const Eigen::Matrix3f& matrix)
{
  //NOTE super fast < 0.1ms. called per frame
  Eigen::JacobiSVD<Eigen::Matrix3f> svd(matrix, Eigen::ComputeFullV | Eigen::ComputeFullU);
  Eigen::Matrix3f R = svd.matrixU() * svd.matrixV().transpose();

  double rx = R(2, 1) - R(1, 2);
  double ry = R(0, 2) - R(2, 0);
  double rz = R(1, 0) - R(0, 1);

  double s = sqrt((rx*rx + ry*ry + rz*rz)*0.25);
  double c = (R.trace() - 1) * 0.5;
  c = c > 1. ? 1. : c < -1. ? -1. : c;

  double theta = acos(c);

  if( s < 1e-5 )
  {
    double t;

    if( c > 0 )
      rx = ry = rz = 0;
    else
    {
      t = (R(0, 0) + 1)*0.5;
      rx = sqrt( std::max(t, 0.0) );
      t = (R(1, 1) + 1)*0.5;
      ry = sqrt( std::max(t, 0.0) ) * (R(0, 1) < 0 ? -1.0 : 1.0);
      t = (R(2, 2) + 1)*0.5;
      rz = sqrt( std::max(t, 0.0) ) * (R(0, 2) < 0 ? -1.0 : 1.0);

      if( fabs(rx) < fabs(ry) && fabs(rx) < fabs(rz) && (R(1, 2) > 0) != (ry*rz > 0) )
        rz = -rz;
      theta /= sqrt(rx*rx + ry*ry + rz*rz);
      rx *= theta;
      ry *= theta;
      rz *= theta;
    }
  }
  else
  {
    double vth = 1/(2*s);
    vth *= theta;
    rx *= vth; ry *= vth; rz *= vth;
  }
  return Eigen::Vector3d(rx, ry, rz).cast<float>();
}

//--------------------------- END OF FUNCS -------------------------------

void ElasticFusion::normaliseDepth(const float & minVal, const float & maxVal)
{
  //NOTE currently not called in GUI - line ~490. should it be??
  std::vector<Uniform> uniforms;

  //NOTE again push_back dead af can't this be done a different way ?
  uniforms.push_back(Uniform("maxVal", maxVal * 1000.f));
  uniforms.push_back(Uniform("minVal", minVal * 1000.f));

  computePacks[ComputePack::NORM]->compute(textures[GPUTexture::DEPTH_RAW]->texture, &uniforms);
}

//NOTE this is not called within run()
//BUT this is what you want to call to save the map
void ElasticFusion::savePly()
{
  std::string filename = saveFilename;
  filename.append(".ply");

  // Open file
  std::ofstream fs;
  fs.open (filename.c_str ());

  Eigen::Vector4f * mapData = globalModel.downloadMap();

  int validCount = 0;

  for(unsigned int i = 0; i < globalModel.lastCount(); i++)
  {
    Eigen::Vector4f pos = mapData[(i * 3) + 0];

    if(pos[3] > confidenceThreshold)
    {
      validCount++;
    }
  }

  // Write header
  fs << "ply";
  fs << "\nformat " << "binary_little_endian" << " 1.0";

  // Vertices
  fs << "\nelement vertex "<< validCount;
  fs << "\nproperty float x"
      "\nproperty float y"
      "\nproperty float z";

  fs << "\nproperty uchar red"
      "\nproperty uchar green"
      "\nproperty uchar blue";

  fs << "\nproperty float nx"
      "\nproperty float ny"
      "\nproperty float nz";

  fs << "\nproperty float radius";

  fs << "\nend_header\n";

  // Close the file
  fs.close ();

  // Open file in binary appendable
  std::ofstream fpout (filename.c_str (), std::ios::app | std::ios::binary);

  for(unsigned int i = 0; i < globalModel.lastCount(); i++)
  {
    Eigen::Vector4f pos = mapData[(i * 3) + 0];

    if(pos[3] > confidenceThreshold)
    {
      Eigen::Vector4f col = mapData[(i * 3) + 1];
      Eigen::Vector4f nor = mapData[(i * 3) + 2];

      nor[0] *= -1;
      nor[1] *= -1;
      nor[2] *= -1;

      float value;
      memcpy (&value, &pos[0], sizeof (float));
      fpout.write (reinterpret_cast<const char*> (&value), sizeof (float));

      memcpy (&value, &pos[1], sizeof (float));
      fpout.write (reinterpret_cast<const char*> (&value), sizeof (float));

      memcpy (&value, &pos[2], sizeof (float));
      fpout.write (reinterpret_cast<const char*> (&value), sizeof (float));

      unsigned char r = int(col[0]) >> 16 & 0xFF;
      unsigned char g = int(col[0]) >> 8 & 0xFF;
      unsigned char b = int(col[0]) & 0xFF;

      fpout.write (reinterpret_cast<const char*> (&r), sizeof (unsigned char));
      fpout.write (reinterpret_cast<const char*> (&g), sizeof (unsigned char));
      fpout.write (reinterpret_cast<const char*> (&b), sizeof (unsigned char));

      memcpy (&value, &nor[0], sizeof (float));
      fpout.write (reinterpret_cast<const char*> (&value), sizeof (float));

      memcpy (&value, &nor[1], sizeof (float));
      fpout.write (reinterpret_cast<const char*> (&value), sizeof (float));

      memcpy (&value, &nor[2], sizeof (float));
      fpout.write (reinterpret_cast<const char*> (&value), sizeof (float));

      memcpy (&value, &nor[3], sizeof (float));
      fpout.write (reinterpret_cast<const char*> (&value), sizeof (float));
    }
  }

  // Close file
  fs.close ();

  delete [] mapData;
}

//NOTE below here are simple get functions --------------------------------------------------------------

//Sad times ahead 
//(NOTE what is this comment above hahahhaha someone didn't like the project)
IndexMap & ElasticFusion::getIndexMap()
{
  return indexMap;
}

GlobalModel & ElasticFusion::getGlobalModel()
{
  return globalModel;
}

Ferns & ElasticFusion::getFerns()
{
  return ferns;
}

Deformation & ElasticFusion::getLocalDeformation()
{
  return localDeformation;
}

std::map<std::string, GPUTexture*> & ElasticFusion::getTextures()
{
  return textures;
}

const std::vector<PoseMatch> & ElasticFusion::getPoseMatches()
{
  return poseMatches;
}

const RGBDOdometry & ElasticFusion::getModelToModel()
{
  return modelToModel;
}

const float & ElasticFusion::getConfidenceThreshold()
{
  return confidenceThreshold;
}

void ElasticFusion::setRgbOnly(const bool & val)
{
  rgbOnly = val;
}

void ElasticFusion::setIcpWeight(const float & val)
{
  icpWeight = val;
}

void ElasticFusion::setPyramid(const bool & val)
{
  pyramid = val;
}

void ElasticFusion::setFastOdom(const bool & val)
{
  fastOdom = val;
}

void ElasticFusion::setSo3(const bool & val)
{
  so3 = val;
}

void ElasticFusion::setFrameToFrameRGB(const bool & val)
{
  frameToFrameRGB = val;
}

void ElasticFusion::setConfidenceThreshold(const float & val)
{
  confidenceThreshold = val;
}

void ElasticFusion::setFernThresh(const float & val)
{
  fernThresh = val;
}

void ElasticFusion::setDepthCutoff(const float & val)
{
  depthCutoff = val;
}

const bool & ElasticFusion::getLost() //lel (@raph have you been here ?)
{
  return lost;
}

const int & ElasticFusion::getTick()
{
  return tick;
}

const int & ElasticFusion::getTimeDelta()
{
  return timeDelta;
}

void ElasticFusion::setTick(const int & val)
{
  tick = val;
}

const float & ElasticFusion::getMaxDepthProcessed()
{
  return maxDepthProcessed;
}

const Eigen::Matrix4f & ElasticFusion::getCurrPose()
{
  return currPose;
}

const int & ElasticFusion::getDeforms()
{
  return deforms;
}

const int & ElasticFusion::getFernDeforms()
{
  return fernDeforms;
}

std::map<std::string, FeedbackBuffer*> & ElasticFusion::getFeedbackBuffers()
{
  return feedbackBuffers;
}

# Research: AI animation (Kimodo, MediaPipe) and Suno terms (2026-10-27)

Web-only research, nothing installed. "UNCONFIRMED" marks what no source settled. Several fetches returned secondary summaries; Suno terms
and NVIDIA/Meta licences should be re-read in full before any money or release depends on them.

## 1. NVIDIA Kimodo
- What: open text-to-motion kinematic diffusion model (released 2026-03-16, arXiv 2603.15546). Inputs: text prompt, duration, optional
  constraints (full-body keyframe poses, end-effector positions, 2D paths/waypoints). Outputs 30 fps, max 10 s (300 frames), 30-joint
  SOMA skeleton (root translation + joint rotations). Checkpoints: SOMA (humans), Unitree G1 (robot), SMPLX.
  Sources: https://research.nvidia.com/labs/sil/projects/kimodo , https://huggingface.co/nvidia/Kimodo-SOMA-SEED-v1.1 , https://arxiv.org/pdf/2603.15546
- Licences: code Apache-2.0; weights licensed per Hugging Face page; the SOMA-SEED checkpoint is under the NVIDIA Open Model License
  and the model card says "ready for commercial use". The SMPLX-RP checkpoint is R&D-only (do not use). Source: https://github.com/nv-tlabs/kimodo
- NVIDIA Open Model License: models commercially usable; worldwide, royalty-free; NVIDIA claims no rights in outputs, user responsible
  for them; do not bypass guardrails (ends licence); suing NVIDIA over the model ends licence; indemnify NVIDIA; if you DISTRIBUTE the
  model itself include licence + Notice file (not needed if only animations ship). Source: https://www.nvidia.com/en-us/agreements/enterprise-software/nvidia-open-model-license/
  (summary via fetch tool; full text not read by me).
- Text encoder: gated `meta-llama/Meta-Llama-3-8B-Instruct` (needs Hugging Face access request + token; adds the Meta Llama 3 licence,
  which I did not read: UNCONFIRMED terms, a replicate wrapper says "Built with Meta Llama 3"). Source: https://research.nvidia.com/labs/sil/projects/kimodo/docs/getting_started/installation.html
- Hardware: ~17 GB VRAM on GPU, mostly the text encoder; `TEXT_ENCODER_DEVICE=cpu` cuts VRAM to under 3 GB, slightly slower. Tested on
  RTX 3090/4090/A100 (model card also lists Ampere/Lovelace/Blackwell). Developed on Linux; "Windows should work especially with
  Docker". PyTorch 2.0+, Python 3.10. Source: https://github.com/nv-tlabs/kimodo , model card.
- RX 580 / no CUDA: no source says Kimodo runs on AMD. It is a PyTorch project built around CUDA GPUs. AMD's official PyTorch-on-Windows
  (ROCm 6.4.4+) lists only RX 9000 / 7000 series and some Ryzen AI APUs; Polaris (RX 580) is not listed (my inference, no source names the
  RX 580). Sources: https://www.amd.com/en/blogs/2025/the-road-to-rocm-on-radeon-for-windows-and-linux.html , https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/compatibility/compatibilityrad/windows/windows_compatibility.html .
  Plain answer: NOT on the RX 580 GPU. Options (none verified end to end): (a) run PyTorch on CPU (i7-8700, 16 GB RAM): the 282M-parameter
  model itself is small, but the Llama-3-8B encoder in fp16 is ~16 GB and will not fit in 16 GB RAM next to Windows; UNCONFIRMED
  whether a quantised/8-bit path exists; (b) hosted: community Replicate deployment https://replicate.com/sprited/kimodo (unofficial;
  BVH export for SOMA, GLB default, FBX only with your own Mixamo-rigged FBX; GPU type and price not stated on the page);
  (c) rent a cloud GPU (Linux + Docker) for batch generation; (d) NVIDIA's web demo/authoring tool exists per the project page (hosting
  location UNCONFIRMED); (e) other open text-to-motion models (MDM, MotionGPT, T2M-GPT) exist but I did not research their licences
  (many are research-only: check each).

### Getting Kimodo motion to Unreal
- Native output is NPZ. BVH export exists for SOMA skeletons only (ComfyUI-Kimodo pack and Replicate wrapper; NVIDIA's own BVH docs
  returned HTTP 403, so not confirmed from NVIDIA). Sources: https://comfy.icu/extension/GuardSkill__ComfyUI-Kimodo-Bridge , https://replicate.com/sprited/kimodo/readme
- Mixamo-FBX retarget: ComfyUI nodes retarget SOMA to a Mixamo-rigged FBX (needs Autodesk FBX SDK Python bindings, Linux/Python 3.12 tested).
- A paid/third-party Fab plugin "Kimodo Importer" imports SOMA BVH into UE and retargets to a bundled UE5 skeleton (not evaluated).
  Source: https://www.fab.com/listings/c77fd26e-39fe-4355-b607-cce8c37de491
- Unreal side: IK Retargeter. Make an IK Rig for the source skeleton (the imported SOMA/Mixamo skeleton) and for ours, define matching retarget
  chains (spine, arms, legs, head), set the pelvis, fix A/T-pose difference with a retarget pose, then "Export Selected Animations" to
  Animation Sequences on our skeleton. Source: https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine
  (the page does not cover BVH import itself; UE FBX import of BVH-converted FBX is the usual path: UNCONFIRMED from sources here).
- Commercial use of the OUTPUT: NVIDIA claims no rights in outputs (above). Copyrightability of AI motion is a general legal question,
  not answered by any source here.

## 2. MediaPipe Pose Landmarker
- What: Google on-device pose estimation, 33 body landmarks per person; normalised image coords plus `WorldLandmarks` (3D in metres, hip
  origin), with visibility. Python: `pip install mediapipe`, Python 3.9+, pip 20.3+, Windows/Mac/Linux listed as supported desktop OS.
  Needs a downloaded model bundle (.task: lite/full/heavy). Sources: https://developers.google.com/edge/mediapipe/solutions/vision/pose_landmarker/python , https://developers.google.com/edge/mediapipe/solutions/setup_python
- Licence: MediaPipe code is Apache 2.0 (Google docs: code samples Apache 2.0, text CC BY 4.0). The licence of the model bundle itself is
  UNCONFIRMED: a Google forum question about commercial use of `pose_landmarker_full` is unanswered
  (https://discuss.ai.google.dev/t/commercial-use-of-mediapipe-pose-landmarker-full/184033). The model card PDF (BlazePose GHUM 3D) could not be read.
  Since we would only use it as a measuring tool and ship only the resulting animation (not the model), the risk is low, but not proven.
- RX 580: MediaPipe pose runs on CPU by default in the Python package (the docs I read do not say; it is commonly CPU-real-time: UNCONFIRMED here). No CUDA needed.
- Output is 33 points (no fingers, no spine detail, noisy depth), not bone rotations: needs solving to a skeleton (IK/aim-at-points), smoothing,
  foot-lock cleanup. Quality is a known weak spot (my judgment).

## 3. The video's method (from CONTRACT5 description; I could not watch the video, steps partly inferred)
1. Make key-pose images (AI image generator): a character in each pose (wind-up, strike, follow-through).
2. Run MediaPipe Pose on each image: 33 landmarks per pose.
3. Convert landmarks to a skeleton pose (bone directions -> joint rotations or Kimodo full-body keyframe constraints).
4. Feed poses as keyframe constraints (Kimodo) with a text prompt, or interpolate between poses yourself, to generate the in-betweens.
5. Export (BVH/FBX), retarget onto the game skeleton, import to Unreal as AnimSequence.
Whether the video used Kimodo's keyframe constraints or its own interpolation is UNCONFIRMED.

## 4. Suno (checked 2026-10-27)
- Free plan: 50 credits/day, no downloads, commercial rights: No (pricing page). Suno keeps ownership of free-plan output; non-commercial
  personal use only. Pro $8/mo (2,500 credits, 20 downloads/mo) and Premier $24/mo (10,000 credits, 60 downloads/mo): commercial rights yes.
  Source: https://suno.com/pricing
- Ownership (Terms): Pro/Premier: Suno assigns its rights in Output to you, still subject to the Terms; Suno gives no warranty that any copyright
  will vest in the Output. Free/Basic: personal non-commercial only. Commercial use only of Output you have DOWNLOADED through an approved
  channel; commercial rights for a Download are perpetual even after cancelling; no retroactive licence for songs made earlier on free;
  removing watermarks/metadata and stream-ripping prohibited. Sources: https://suno.com/terms , https://help.suno.com/en/articles/2416769 ,
  https://www.musicinafrica.net/magazine/suno-adjusts-ai-music-ownership-terms-after-warner-music-partnership
- Voices: you may create a Voice Model only of your own voice (Terms). Voices/Personas are Pro/Premier features. No NPC-voice or speech
  (text-to-speech) product is described in the sources I read; Suno is a song generator (sung vocals).
- Sound effects: "Suno Sounds" (beta) makes short effects/loops/ambience from text, per third-party guides; Suno's own terms and pricing page
  do not mention sound effects, and I found no official statement that Sounds output carries the same commercial grant: UNCONFIRMED, ask
  Suno support. Source: https://jackrighteous.com/en-us/blogs/guides-using-suno-ai-music-creation/suno-sounds-ai-soundeffects-guide
- Other cautions: record labels' lawsuits against Suno reported (secondary source, April 2026; may be settled/changed since: the Warner
  partnership article above suggests terms changed); no indemnification in terms (secondary source, https://listir.is/tools/suno).
- Verdict for us: FREE plan = NOT usable in a Steam game. Paid (Pro, $8/mo) allows commercial use of downloaded songs. Rule 1 in CLAUDE.md
  ("no copyrighted content", sounds are script-made) is in tension with an AI music tool trained on undisclosed data; the user decides.

## What this would take for us
- Kimodo here: not on the RX 580. Realistic path = rent/borrow an NVIDIA GPU (cloud Linux + Docker, or the Replicate wrapper), generate SOMA
  BVH clips offline, bring BVHs home. Needs Hugging Face account + Llama 3 access approval. Cost per clip UNCONFIRMED.
- Our side: one-time IK Rig + Retargeter from SOMA (or Mixamo) skeleton to our player/creature skeleton (our skeletons are code/Blender
  made, so chain names must be mapped by hand), then Export Animation Sequences. Pipeline script for BVH -> FBX (Blender can import BVH:
  general knowledge, not sourced here) -> UE import. Creature rigs (spiders, slimes) do not match a human SOMA skeleton: humanoids only.
- Cleanup budget: foot sliding, hand/finger poses (SOMA has few finger joints?: UNCONFIRMED), loops, root motion vs our capsule.
- MediaPipe route: free, Windows, CPU, no licence worry for code; but gives only 33 points, so we'd write the landmark->rotation solver ourselves.
- Compare with what we have: our melee/hand motions are procedural code; Kimodo mainly helps humanoid NPC/boss full-body animation.
- Legal to-do before shipping any of it: read full NVIDIA Open Model License, Llama 3 licence, MediaPipe model-card licence; keep notes in repo.

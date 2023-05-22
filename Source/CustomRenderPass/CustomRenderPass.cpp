// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomRenderPass.h"
#include "Modules/ModuleManager.h"

class ProjectMainModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		FString projectDir = FPaths::ProjectDir();
		FString projectShaderDir = FPaths::Combine(projectDir, TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Project"), projectShaderDir);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(ProjectMainModule, CustomRenderPass, "CustomRenderPass" );

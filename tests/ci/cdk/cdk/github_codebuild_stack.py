# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import typing
from util.util import EnvUtil

from aws_cdk import core, aws_codebuild as codebuild, aws_iam as iam, aws_ecr as ecr


class GitHubCodeBuildStack(core.Stack):
    """Define a stack used to run AWS-LC tests."""

    def __init__(self,
                 scope: core.Construct,
                 id: str,
                 ecr_repo_name: str,
                 docker_img_tag: str,
                 build_spec_file: str,
                 is_windows: typing.Optional[bool] = False,
                 privileged: typing.Optional[bool] = False,
                 **kwargs) -> None:
        super().__init__(scope, id, **kwargs)

        # Fetch environment variables.
        github_repo_owner = EnvUtil.get("GITHUB_REPO_OWNER", "awslabs")
        github_repo = EnvUtil.get("GITHUB_REPO", "aws-lc")

        # Define CodeBuild resource.
        git_hub_source = codebuild.Source.git_hub(
            owner=github_repo_owner,
            repo=github_repo,
            webhook=True,
            webhook_filters=[
                codebuild.FilterGroup.in_event_of(
                    codebuild.EventAction.PULL_REQUEST_CREATED,
                    codebuild.EventAction.PULL_REQUEST_UPDATED,
                    codebuild.EventAction.PULL_REQUEST_REOPENED)
            ],
            clone_depth=1)

        # Define CodeBuild environment.
        ecr_repo = ecr.Repository.from_repository_name(scope=self, id=ecr_repo_name, repository_name=ecr_repo_name)
        build_image = codebuild.LinuxBuildImage.from_ecr_repository(repository=ecr_repo, tag=docker_img_tag)
        if is_windows:
            build_image = codebuild.WindowsBuildImage.from_ecr_repository(repository=ecr_repo, tag=docker_img_tag)

        # Define a role.
        role = iam.Role(scope=self,
                        id="{}-role".format(id),
                        assumed_by=iam.ServicePrincipal("codebuild.amazonaws.com"),
                        managed_policies=[
                            iam.ManagedPolicy.from_aws_managed_policy_name("AmazonEC2ContainerRegistryReadOnly")
                        ])

        # Define CodeBuild.
        codebuild.Project(
            scope=self,
            id=id,
            project_name=id,
            source=git_hub_source,
            role=role,
            environment=codebuild.BuildEnvironment(compute_type=codebuild.ComputeType.LARGE,
                                                   privileged=privileged,
                                                   build_image=build_image),
            build_spec=codebuild.BuildSpec.from_source_filename(build_spec_file))

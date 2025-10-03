import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query'
import { DockerApiService } from '@/services/api'

export const useDockerInfo = () => {
  return useQuery({
    queryKey: ['docker', 'info'],
    queryFn: DockerApiService.getDockerInfo,
  })
}

export const useDockerComposeInfo = () => {
  return useQuery({
    queryKey: ['docker-compose', 'info'],
    queryFn: DockerApiService.getDockerComposeInfo,
  })
}

export const useInstallationProgress = (enabled: boolean = false) => {
  return useQuery({
    queryKey: ['docker', 'installation', 'progress'],
    queryFn: DockerApiService.getInstallationProgress,
    enabled,
    refetchInterval: enabled ? 2000 : false,
  })
}

export const useDockerServiceStatus = (enabled: boolean = true) => {
  return useQuery({
    queryKey: ['docker', 'service', 'status'],
    queryFn: DockerApiService.getDockerServiceStatus,
    enabled,
    // refetchInterval: enabled ? 10000 : false, // Reduced from 5s to 10s
    refetchInterval: false, // Disabled Interval
  })
}

export const useContainers = (all: boolean = false) => {
  return useQuery({
    queryKey: ['docker', 'containers', all],
    queryFn: () => DockerApiService.listContainers(all),
  })
}

export const useDetailedContainers = (all: boolean = false) => {
  return useQuery({
    queryKey: ['docker', 'containers', 'detailed', all],
    queryFn: () => DockerApiService.getDetailedContainers(all),
  })
}

export const useImages = () => {
  return useQuery({
    queryKey: ['docker', 'images'],
    queryFn: DockerApiService.listImages,
  })
}

export const useSystemInfo = () => {
  return useQuery({
    queryKey: ['docker', 'system', 'info'],
    queryFn: DockerApiService.getSystemInfo,
  })
}

export const useDiskUsage = () => {
  return useQuery({
    queryKey: ['docker', 'system', 'df'],
    queryFn: DockerApiService.getDiskUsage,
  })
}

export const useSystemIntegrationStatus = () => {
  return useQuery({
    queryKey: ['docker', 'system', 'integration-status'],
    queryFn: DockerApiService.getSystemIntegrationStatus,
  })
}

export const useComposeProjects = () => {
  return useQuery({
    queryKey: ['docker', 'compose', 'projects'],
    queryFn: DockerApiService.listComposeProjects,
  })
}

export const useComposeServices = (projectName: string) => {
  return useQuery({
    queryKey: ['docker', 'compose', 'projects', projectName, 'services'],
    queryFn: () => DockerApiService.getComposeServices(projectName),
    enabled: !!projectName,
  })
}

export const useContainerLogs = (containerId: string, lines: number = 100) => {
  return useQuery({
    queryKey: ['docker', 'containers', containerId, 'logs', lines],
    queryFn: () => DockerApiService.getContainerLogs(containerId, lines),
    enabled: !!containerId,
  })
}

// Mutations
export const useInstallDocker = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.installDocker,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker'] })
    },
  })
}

export const useUninstallDocker = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.uninstallDocker,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker'] })
    },
  })
}

export const useStartDockerService = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.startDockerService,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'service'] })
    },
    onError(error, variables, context) {
      alert(error);
    }
  })
}

export const useStopDockerService = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.stopDockerService,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'service'] })
    },
  })
}

export const useRestartDockerService = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.restartDockerService,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'service'] })
    },
  })
}

export const useEnableDockerService = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.enableDockerService,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'service'] })
    },
  })
}

export const useDisableDockerService = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.disableDockerService,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'service'] })
    },
  })
}

export const useStartContainer = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.startContainer,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'containers'] })
    },
  })
}

export const useStopContainer = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: ({ containerId, timeout }: { containerId: string; timeout?: number }) =>
      DockerApiService.stopContainer(containerId, timeout),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'containers'] })
    },
  })
}

export const useRestartContainer = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: ({ containerId, timeout }: { containerId: string; timeout?: number }) =>
      DockerApiService.restartContainer(containerId, timeout),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'containers'] })
    },
  })
}

export const usePauseContainer = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.pauseContainer,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'containers'] })
    },
  })
}

export const useUnpauseContainer = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.unpauseContainer,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'containers'] })
    },
  })
}

export const useRemoveContainer = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: ({ containerId, force }: { containerId: string; force?: boolean }) =>
      DockerApiService.removeContainer(containerId, force),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'containers'] })
    },
  })
}

export const usePullImage = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.pullImage,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'images'] })
    },
  })
}

export const useRemoveImage = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: ({ imageId, force }: { imageId: string; force?: boolean }) =>
      DockerApiService.removeImage(imageId, force),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'images'] })
    },
  })
}

export const useLoadImage = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.loadImage,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'images'] })
    },
  })
}

export const useSaveImage = () => {
  return useMutation({
    mutationFn: DockerApiService.saveImage,
  })
}

export const useTagImage = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.tagImage,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'images'] })
    },
  })
}

export const useBuildImage = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.buildImage,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'images'] })
    },
  })
}

export const usePruneSystem = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.pruneSystem,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker'] })
    },
  })
}

export const useLoadComposeProject = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.loadComposeProject,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'compose'] })
    },
  })
}

export const useRemoveComposeProject = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.removeComposeProject,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'compose'] })
    },
  })
}

export const useStartComposeProject = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: ({ projectName, detached }: { projectName: string; detached?: boolean }) =>
      DockerApiService.startComposeProject(projectName, detached),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'compose'] })
    },
  })
}

export const useStopComposeProject = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: ({ projectName, removeVolumes }: { projectName: string; removeVolumes?: boolean }) =>
      DockerApiService.stopComposeProject(projectName, removeVolumes),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'compose'] })
    },
  })
}

export const useRestartComposeProject = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: DockerApiService.restartComposeProject,
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'compose'] })
    },
  })
}

export const useExecuteServiceAction = () => {
  const queryClient = useQueryClient()
  return useMutation({
    mutationFn: ({ projectName, serviceName, action }: { projectName: string; serviceName: string; action: string }) =>
      DockerApiService.executeServiceAction(projectName, serviceName, action),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ['docker', 'compose'] })
    },
  })
}

export const useTestSudo = () => {
  return useMutation({
    mutationFn: DockerApiService.testSudo,
  })
}
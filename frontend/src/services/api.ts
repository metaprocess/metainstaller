import axios, { AxiosResponse } from 'axios'
import { getApiUrl } from '@/utils/env'
import type {
  DockerInfo,
  DockerComposeInfo,
  ApiResponse,
  Container,
  Image,
  SystemInfo,
  DiskUsage,
  InstallationProgress,
  SystemIntegrationStatus,
  ComposeProject,
  ServiceStatus,
  ContainerLogs,
  PullImageRequest,
  LoadImageRequest,
  SaveImageRequest,
  TagImageRequest,
  BuildImageRequest,
  ComposeProjectRequest,
  SudoTestRequest,
  ProjectInfo,
  ProjectArchiveInfo,
  ProjectStatus,
  AnalyzeArchiveRequest,
  LoadProjectRequest,
  CreateArchiveRequest,
  RemoveProjectRequest,
} from '@/types/api'

const api = axios.create({
  timeout: 30000,
})

export class DockerApiService {
  // Docker Information
  static async getDockerInfo(): Promise<DockerInfo> {
    const response: AxiosResponse<DockerInfo> = await api.get(getApiUrl('/api/docker/info'))
    return response.data
  }

  static async getDockerComposeInfo(): Promise<DockerComposeInfo> {
    const response: AxiosResponse<DockerComposeInfo> = await api.get(getApiUrl('/api/docker-compose/info'))
    return response.data
  }

  // Installation
  static async installDocker(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(getApiUrl('/api/docker/install'))
    return response.data
  }

  static async uninstallDocker(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.delete(getApiUrl('/api/docker/uninstall'))
    return response.data
  }

  static async getInstallationProgress(): Promise<InstallationProgress> {
    const response: AxiosResponse<InstallationProgress> = await api.get(getApiUrl('/api/docker/installation/progress'))
    return response.data
  }

  // Service Management
  static async startDockerService(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(getApiUrl('/api/docker/service/start'))
    return response.data
  }

  static async stopDockerService(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(getApiUrl('/api/docker/service/stop'))
    return response.data
  }

  static async restartDockerService(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(getApiUrl('/api/docker/service/restart'))
    return response.data
  }

  static async getDockerServiceStatus(): Promise<ServiceStatus> {
    const response: AxiosResponse<ServiceStatus> = await api.get(getApiUrl('/api/docker/service/status'))
    return response.data
  }

  static async enableDockerService(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(getApiUrl('/api/docker/service/enable'))
    return response.data
  }

  static async disableDockerService(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(getApiUrl('/api/docker/service/disable'))
    return response.data
  }

  // Container Management
  static async listContainers(all: boolean = false): Promise<{ containers: Container[] }> {
    const response: AxiosResponse<{ containers: Container[] }> = await api.get(
      getApiUrl(`/api/docker/containers?all=${all}`)
    )
    return response.data
  }

  static async getDetailedContainers(all: boolean = false): Promise<{ containers: Container[] }> {
    const response: AxiosResponse<{ containers: Container[] }> = await api.get(
      getApiUrl(`/api/docker/containers/detailed?all=${all}`)
    )
    return response.data
  }

  static async startContainer(containerId: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/docker/containers/${containerId}/start`)
    )
    return response.data
  }

  static async stopContainer(containerId: string, timeout?: number): Promise<ApiResponse> {
    const url = timeout ? 
      getApiUrl(`/api/docker/containers/${containerId}/stop?timeout=${timeout}`) :
      getApiUrl(`/api/docker/containers/${containerId}/stop`)
    const response: AxiosResponse<ApiResponse> = await api.post(url)
    return response.data
  }

  static async restartContainer(containerId: string, timeout?: number): Promise<ApiResponse> {
    const url = timeout ? 
      getApiUrl(`/api/docker/containers/${containerId}/restart?timeout=${timeout}`) :
      getApiUrl(`/api/docker/containers/${containerId}/restart`)
    const response: AxiosResponse<ApiResponse> = await api.post(url)
    return response.data
  }

  static async pauseContainer(containerId: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/docker/containers/${containerId}/pause`)
    )
    return response.data
  }

  static async unpauseContainer(containerId: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/docker/containers/${containerId}/unpause`)
    )
    return response.data
  }

  static async removeContainer(containerId: string, force: boolean = false): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.delete(
      getApiUrl(`/api/docker/containers/${containerId}?force=${force}`)
    )
    return response.data
  }

  static async getContainerLogs(containerId: string, lines: number = 200): Promise<ContainerLogs> {
    const response: AxiosResponse<ContainerLogs> = await api.get(
      getApiUrl(`/api/docker/containers/${containerId}/logs?lines=${lines}`)
    )
    return response.data
  }

  // Image Management
  static async listImages(): Promise<{ images: Image[] }> {
    const response: AxiosResponse<{ images: Image[] }> = await api.get(getApiUrl('/api/docker/images'))
    return response.data
  }

  static async pullImage(request: PullImageRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/docker/images/pull'),
      request
    )
    return response.data
  }

  static async removeImage(imageId: string, force: boolean = false): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.delete(
      getApiUrl(`/api/docker/images/${imageId}?force=${force}`)
    )
    return response.data
  }

  static async loadImage(request: LoadImageRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/docker/images/load'),
      request
    )
    return response.data
  }

  static async saveImage(request: SaveImageRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/docker/images/save'),
      request
    )
    return response.data
  }

  static async tagImage(request: TagImageRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/docker/images/tag'),
      request
    )
    return response.data
  }

  static async buildImage(request: BuildImageRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/docker/images/build'),
      request
    )
    return response.data
  }

  // System Information
  static async getSystemInfo(): Promise<SystemInfo> {
    const response: AxiosResponse<SystemInfo> = await api.get(getApiUrl('/api/docker/system/info'))
    return response.data
  }

  static async getDiskUsage(): Promise<DiskUsage[]> {
    const response: AxiosResponse<DiskUsage[]> = await api.get(getApiUrl('/api/docker/system/df'))
    return response.data
  }

  static async pruneSystem(): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(getApiUrl('/api/docker/system/prune'))
    return response.data
  }

  static async getSystemIntegrationStatus(): Promise<SystemIntegrationStatus> {
    const response: AxiosResponse<SystemIntegrationStatus> = await api.get(
      getApiUrl('/api/docker/system/integration-status')
    )
    return response.data
  }

  // Docker Compose
  static async loadComposeProject(request: ComposeProjectRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/docker/compose/projects'),
      request
    )
    return response.data
  }

  static async listComposeProjects(): Promise<{ projects: ComposeProject[] }> {
    const response: AxiosResponse<{ projects: ComposeProject[] }> = await api.get(
      getApiUrl('/api/docker/compose/projects')
    )
    return response.data
  }

  static async removeComposeProject(projectName: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.delete(
      getApiUrl(`/api/docker/compose/projects/${projectName}`)
    )
    return response.data
  }

  static async startComposeProject(projectName: string, detached: boolean = true): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/docker/compose/projects/${projectName}/up?detached=${detached}`)
    )
    return response.data
  }

  static async stopComposeProject(projectName: string, removeVolumes: boolean = false): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/docker/compose/projects/${projectName}/down?remove_volumes=${removeVolumes}`)
    )
    return response.data
  }

  static async restartComposeProject(projectName: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/docker/compose/projects/${projectName}/restart`)
    )
    return response.data
  }

  static async getComposeServices(projectName: string): Promise<{ project_name: string; services: string[] }> {
    const response: AxiosResponse<{ project_name: string; services: string[] }> = await api.get(
      getApiUrl(`/api/docker/compose/projects/${projectName}/services`)
    )
    return response.data
  }

  static async executeServiceAction(
    projectName: string,
    serviceName: string,
    action: string
  ): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/docker/compose/projects/${projectName}/services/${serviceName}/${action}`),
      { action }
    )
    return response.data
  }

  // Test
  static async testSudo(request: SudoTestRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/test/sudo'),
      request
    )
    return response.data
  }

  // Project Management
  static async analyzeArchive(request: AnalyzeArchiveRequest): Promise<ProjectArchiveInfo> {
    const response: AxiosResponse<ProjectArchiveInfo> = await api.post(
      getApiUrl('/api/projects/analyze'),
      request
    )
    return response.data
  }

  static async loadProject(request: LoadProjectRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/projects/load'),
      request
    )
    return response.data
  }

  static async unloadProject(projectName: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/projects/${projectName}/unload`)
    )
    return response.data
  }

  static async removeProject(projectName: string, request: RemoveProjectRequest = {}): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.delete(
      getApiUrl(`/api/projects/${projectName}/remove`),
      { data: request }
    )
    return response.data
  }

  static async startProject(projectName: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/projects/${projectName}/start`)
    )
    return response.data
  }

  static async stopProject(projectName: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/projects/${projectName}/stop`)
    )
    return response.data
  }

  static async restartProject(projectName: string): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl(`/api/projects/${projectName}/restart`)
    )
    return response.data
  }

  static async listProjects(): Promise<{ success: boolean; projects: ProjectInfo[] }> {
    const response: AxiosResponse<{ success: boolean; projects: ProjectInfo[] }> = await api.get(
      getApiUrl('/api/projects')
    )
    return response.data
  }

  static async getProjectInfo(projectName: string): Promise<{ success: boolean; project: ProjectInfo }> {
    const response: AxiosResponse<{ success: boolean; project: ProjectInfo }> = await api.get(
      getApiUrl(`/api/projects/${projectName}`)
    )
    return response.data
  }

  static async getProjectServices(projectName: string): Promise<{ success: boolean; project_name: string; services: string[] }> {
    const response: AxiosResponse<{ success: boolean; project_name: string; services: string[] }> = await api.get(
      getApiUrl(`/api/projects/${projectName}/services`)
    )
    return response.data
  }

  static async getProjectLogs(projectName: string, serviceName?: string): Promise<{ success: boolean; project_name: string; service_name: string; logs: string }> {
    const url = serviceName 
      ? getApiUrl(`/api/projects/${projectName}/logs?service=${serviceName}`)
      : getApiUrl(`/api/projects/${projectName}/logs`)
    const response: AxiosResponse<{ success: boolean; project_name: string; service_name: string; logs: string }> = await api.get(url)
    return response.data
  }

  static async createProjectArchive(request: CreateArchiveRequest): Promise<ApiResponse> {
    const response: AxiosResponse<ApiResponse> = await api.post(
      getApiUrl('/api/projects/create-archive'),
      request
    )
    return response.data
  }

  static async getProjectStatus(projectName: string): Promise<ProjectStatus> {
    const response: AxiosResponse<ProjectStatus> = await api.get(
      getApiUrl(`/api/projects/${projectName}/status`)
    )
    return response.data
  }

  // Version information
  static async getVersion(): Promise<{ version: string }> {
    const response: AxiosResponse<{ version: string }> = await api.get(getApiUrl('/api/version'))
    return response.data
  }
}

export interface DockerInfo {
  installed: boolean
  version?: string
  api_version?: string
  min_api_version?: string
  git_commit?: string
  go_version?: string
  os?: string
  arch?: string
  kernel_version?: string
  experimental?: boolean
  build_time?: string
  error_message?: string
}

export interface DockerComposeInfo {
  installed: boolean
  version?: string
  error_message?: string
}

export interface ApiResponse<T = any> {
  success?: boolean
  message?: string
  data?: T
}

export interface Container {
  id: string
  name: string
  image: string
  created: string
  status: string
  ports: string | any[]
}

export interface Image {
  id: string
  repository: string
  tag: string
  size: string
  created: string
}

export interface SystemInfo {
  ID: string
  Containers: number
  ContainersRunning: number
  ContainersPaused: number
  ContainersStopped: number
  Images: number
  Driver: string
  DriverStatus: string[][]
  Plugins: {
    Volume: string[]
    Network: string[]
  }
  MemoryLimit: boolean
  SwapLimit: boolean
  KernelMemoryTCP: boolean
  CpuCfsPeriod: boolean
  CpuCfsQuota: boolean
  CPUShares: boolean
  CPUSet: boolean
  PidsLimit: boolean
  IPv4Forwarding: boolean
  Debug: boolean
  NFd: number
  OomKillDisable: boolean
  NGoroutines: number
  SystemTime: string
  LoggingDriver: string
  CgroupDriver: string
  CgroupVersion: string
  NEventsListener: number
  KernelVersion: string
  OperatingSystem: string
  OSVersion: string
  OSType: string
  Architecture: string
  IndexServerAddress: string
  NCPU: number
  MemTotal: number
  DockerRootDir: string
  HttpProxy: string
  HttpsProxy: string
  NoProxy: string
  Name: string
  Labels: string[]
  ExperimentalBuild: boolean
  ServerVersion: string
  DefaultRuntime: string
  LiveRestoreEnabled: boolean
  Isolation: string
  InitBinary: string
  ProductLicense: string
}

export interface DiskUsage {
  Type: string
  TotalCount: string
  Active: string
  Size: string
  Reclaimable: string
}

export interface InstallationProgress {
  installation_in_progress: boolean
  error_details: string
  message: string
  percentage: number
  status: number
}

export interface SystemIntegrationStatus {
  docker_running: boolean
  docker_socket_enabled: boolean
  docker_group_exists: boolean
  docker_service_enabled: boolean
  systemd_services_installed: boolean
  fully_integrated: boolean
  binaries_installed: boolean
}

export interface ComposeProject {
  name: string
  compose_file_path: string
  working_directory: string
  services: string[]
}

export interface ServiceStatus {
  status: string
  running: boolean
}

export interface ContainerLogs {
  container_id: string
  lines: number
  logs: string
}

export interface PullImageRequest {
  image: string
}

export interface LoadImageRequest {
  file_path: string
}

export interface SaveImageRequest {
  image_name: string
  file_path: string
}

export interface TagImageRequest {
  source_image: string
  target_tag: string
}

export interface BuildImageRequest {
  dockerfile_path: string
  image_name: string
  build_context: string
}

export interface ComposeProjectRequest {
  compose_file_path: string
  project_name: string
  working_directory: string
}

export interface ServiceActionRequest {
  action: string
}

export interface SudoTestRequest {
  password: string
}

// Project Management Types
export interface ProjectInfo {
  name: string
  archive_path: string
  extracted_path: string
  compose_file_path: string
  is_loaded: boolean
  is_running: boolean
  status_message: string
  created_time: string
  last_modified: string
  required_images: string[]
  dependent_files: string[]
}

export interface ProjectArchiveInfo {
  success: boolean
  archive_path: string
  is_encrypted: boolean
  integrity_verified: boolean
  contained_files: string[]
  docker_images: string[]
  error_message: string
}

export interface ProjectStatus {
  success: boolean
  project_name: string
  status: string
}

export interface AnalyzeArchiveRequest {
  archive_path: string
  password?: string
}

export interface LoadProjectRequest {
  archive_path: string
  project_name: string
  password?: string
}

export interface CreateArchiveRequest {
  project_path: string
  archive_path: string
  password?: string
}

export interface RemoveProjectRequest {
  remove_files?: boolean
}
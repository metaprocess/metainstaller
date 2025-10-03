import { NavLink } from 'react-router-dom'
import {
  Container,
  HardDrive,
  FileText,
  Settings,
  Activity,
  Terminal,
  Download,
  Server,
  Archive,
  ChevronLeft,
  ChevronRight,
  Menu
} from 'lucide-react'
import { cn } from '@/utils/cn'
import { useState, useEffect } from 'react'
import { DockerApiService } from '@/services/api'

const Navigation = () => {
  const [backendVersion, setBackendVersion] = useState<string>('')
  const [isExpanded, setIsExpanded] = useState(true)
  const frontendVersion = import.meta.env.VITE_METAINSTALLER_VERSION || 'Unknown'

  const toggleNavigation = () => {
    setIsExpanded(!isExpanded)
  }

  useEffect(() => {
    const fetchBackendVersion = async () => {
      try {
        const versionData = await DockerApiService.getVersion()
        setBackendVersion(versionData.version)
      } catch (error) {
        console.error('Failed to fetch backend version:', error)
        setBackendVersion('Unknown')
      }
    }

    fetchBackendVersion()
  }, [])

  const navItems = [
    { to: '/', icon: Activity, label: 'Dashboard' },
    { to: '/installation', icon: Download, label: 'Installation' },
    { to: '/service', icon: Server, label: 'Service' },
    { to: '/containers', icon: Container, label: 'Containers' },
    { to: '/images', icon: HardDrive, label: 'Images' },
    { to: '/projects', icon: Archive, label: 'MetaProjects' },
    { to: '/system', icon: Settings, label: 'System' },
    // { to: '/logs', icon: Terminal, label: 'Logs' },
  ]

  return (
    <nav className={cn(
      "bg-white border-r border-gray-200 h-full flex flex-col transition-all duration-300",
      isExpanded ? "w-64" : "w-16"
    )}>
      <div className="p-6 flex items-center justify-between">
        <h1 className={cn(
          "text-xl font-bold text-gray-900 transition-opacity duration-300",
          isExpanded ? "opacity-100" : "opacity-0"
        )}>
          METAINSTALLER
          {(backendVersion || frontendVersion) && isExpanded && (
            <div className="space-y-1 mt-2">
              {backendVersion && (
                <div>
                  <span className="font-semibold bg-blue-100 text-xs text-blue-800 rounded-full">BV:{backendVersion}</span>
                </div>
              )}
              {frontendVersion && (
                <div>
                  <span className="font-semibold bg-green-100 text-xs text-green-800 rounded-full">FV:{frontendVersion}</span>
                </div>
              )}
            </div>
          )}
        </h1>
        {isExpanded && (
          <button
            onClick={toggleNavigation}
            className="p-1 rounded-md hover:bg-gray-100 transition-colors"
            aria-label="Collapse navigation"
          >
            <ChevronLeft className="h-5 w-5" />
          </button>
        )}
      </div>
      {/* {version && (
        <div className="mt-2 flex justify-center">
          <div className="bg-blue-100 text-blue-800 text-xs font-medium px-3 py-1 rounded-full">
            <span className="font-semibold">v{version}</span>
          </div>
        </div>
      )} */}

      <div className="flex-1 px-2 pb-4">
        {!isExpanded && (
          <div className="flex justify-center py-2">
            <button
              onClick={toggleNavigation}
              className="p-1 rounded-md hover:bg-gray-100 transition-colors"
              aria-label="Expand navigation"
            >
              <Menu className="h-5 w-5" />
            </button>
          </div>
        )}
        <div className="border-t border-gray-200 my-2" />
        <ul className="space-y-2">
          {navItems.map((item) => {
            const Icon = item.icon
            return (
              <li key={item.to}>
                <NavLink
                  to={item.to}
                  className={({ isActive }) =>
                    cn(
                      'flex items-center px-3 py-2 text-sm font-medium rounded-lg transition-colors',
                      isActive
                        ? 'bg-blue-100 text-blue-700'
                        : 'text-gray-700 hover:bg-gray-100',
                      isExpanded ? 'justify-start' : 'justify-center'
                    )
                  }
                >
                  <Icon className="h-5 w-5" />
                  {isExpanded && <span className="ml-3">{item.label}</span>}
                </NavLink>
              </li>
            )
          })}
        </ul>
      </div>
    </nav>
  )
}

export default Navigation
